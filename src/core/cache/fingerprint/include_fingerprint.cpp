#include "beez/core/cache/fingerprint/include_fingerprint.hpp"

#include "beez/core/cache/fingerprint/content_hash.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace beez::core
{

namespace
{

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- path parameters are semantically distinct

[[nodiscard]] std::optional<std::string_view> parseQuotedInclude(std::string_view line)
{
    const auto HashPos = line.find('#');
    if (HashPos == std::string_view::npos)
    {
        return std::nullopt;
    }

    const auto IncludePos = line.find("include", HashPos);
    if (IncludePos == std::string_view::npos)
    {
        return std::nullopt;
    }

    const auto QuotePos = line.find('"', IncludePos);
    if (QuotePos == std::string_view::npos)
    {
        return std::nullopt;
    }

    const auto EndQuotePos = line.find('"', QuotePos + 1);
    if (EndQuotePos == std::string_view::npos)
    {
        return std::nullopt;
    }

    return line.substr(QuotePos + 1, EndQuotePos - QuotePos - 1);
}

[[nodiscard]] std::optional<std::filesystem::path>
resolveIncludePath(const std::filesystem::path& includePath,
                   const std::filesystem::path& includingFile,
                   const std::filesystem::path& projectRoot)
{
    const auto RelativeToIncluding = includingFile.parent_path() / includePath;
    if (std::filesystem::exists(RelativeToIncluding))
    {
        return std::filesystem::canonical(RelativeToIncluding);
    }

    const auto RelativeToProject = projectRoot / includePath;
    if (std::filesystem::exists(RelativeToProject))
    {
        return std::filesystem::canonical(RelativeToProject);
    }

    return std::nullopt;
}

void collectIncludeTree(const std::filesystem::path& sourcePath,
                        const std::filesystem::path& projectRoot,
                        const IContentHasher& hasher,
                        std::set<std::filesystem::path>& visited,
                        std::vector<std::string>& parts)
{
    std::vector<std::filesystem::path> pending {sourcePath};

    while (!pending.empty())
    {
        const auto Current = pending.back();
        pending.pop_back();

        std::error_code errorCode;
        if (!std::filesystem::is_regular_file(Current, errorCode))
        {
            continue;
        }

        const auto Canonical = std::filesystem::weakly_canonical(Current, errorCode);
        if (!visited.insert(Canonical).second)
        {
            continue;
        }

        parts.push_back(Canonical.generic_string() + '\0' + hasher.hashFile(Canonical));

        std::ifstream stream(Canonical);
        if (!stream.is_open())
        {
            continue;
        }

        std::string line;
        while (std::getline(stream, line))
        {
            const auto IncludePath = parseQuotedInclude(line);
            if (!IncludePath.has_value())
            {
                continue;
            }

            const auto Resolved =
                resolveIncludePath(std::filesystem::path(*IncludePath), Canonical, projectRoot);
            if (!Resolved.has_value())
            {
                continue;
            }

            pending.push_back(*Resolved);
        }
    }
}

// NOLINTEND(bugprone-easily-swappable-parameters)

}  // namespace

std::string includeTreeFingerprint(const std::filesystem::path& sourcePath,
                                   const std::filesystem::path& projectRoot,
                                   const IContentHasher& hasher)
{
    std::set<std::filesystem::path> visited;
    std::vector<std::string> parts;
    collectIncludeTree(sourcePath, projectRoot, hasher, visited, parts);
    std::ranges::sort(parts);

    std::ostringstream combined;
    for (const auto& part : parts)
    {
        combined << part << '\0';
    }

    return hasher.hashBytes(combined.str());
}

}  // namespace beez::core
