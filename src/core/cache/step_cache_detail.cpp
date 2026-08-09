#include "step_cache_detail.hpp"

#include "beez/core/cache/content_hash.hpp"
#include "beez/core/cache/storage.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ranges>  // NOLINT(misc-include-cleaner) -- std::ranges algorithms
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace beez::core::step_cache_detail
{

[[nodiscard]] bool outputsExist(const std::vector<std::string>& outputs,
                                const std::filesystem::path& projectRoot)
{
    if (outputs.empty())
    {
        return false;
    }

    return std::ranges::all_of(outputs,
                               [&projectRoot](const std::string& relativePath)
                               { return std::filesystem::exists(projectRoot / relativePath); });
}

[[nodiscard]] bool stepHasArtifacts(const Step& step)
{
    return !step.input.empty() || !step.output.empty() || !step.mutate.empty();
}

[[nodiscard]] std::string stepExecutionIdentity(const Step& step)
{
    std::ostringstream stream;
    stream << step.name << '\0' << step.phase << '\0' << step.scope << '\0';
    if (step.shellRun.has_value())
    {
        stream << *step.shellRun;
    }
    else if (step.hasCallback())
    {
        stream << "<callback>";
    }
    return stream.str();
}

[[nodiscard]] std::string configFingerprint(const StepConfigPtr& config)
{
    if (config == nullptr || config->empty())
    {
        return {};
    }
    return config->cacheFingerprint();
}

[[nodiscard]] std::vector<std::string> artifactPatternsForInputs(const Step& step)
{
    std::vector<std::string> patterns = step.input;
    patterns.insert(patterns.end(), step.mutate.begin(), step.mutate.end());
    return patterns;
}

[[nodiscard]] std::string buildScriptFingerprint(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher)
{
    if (!step.hasCallback())
    {
        return {};
    }

    return hasher.hashFile(projectRoot / "build.lua");
}

[[nodiscard]] std::string stepCommandFingerprint(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher)
{
    if (step.shellRun.has_value())
    {
        return *step.shellRun;
    }

    if (step.hasCallback())
    {
        return std::string("<callback>:") + buildScriptFingerprint(step, projectRoot, hasher);
    }

    return {};
}

[[nodiscard]] std::string sanitizeIndexComponent(std::string value)
{
    std::ranges::replace_if(
        value,
        [](const char Character)
        { return Character == '/' || Character == ':' || Character == '\\'; },
        '_');
    return value;
}

[[nodiscard]] std::filesystem::path indexPathForStep(const std::filesystem::path& indexRoot,
                                                     const Step& step)
{
    const std::string FileName = sanitizeIndexComponent(step.name) + "__" +
                                 sanitizeIndexComponent(step.phase) + "__" +
                                 sanitizeIndexComponent(step.scope) + ".index";
    return indexRoot / FileName;
}

[[nodiscard]] std::vector<InputStamp> collectInputStamps(const Step& step,
                                                         const std::filesystem::path& projectRoot,
                                                         const IGlobMatcher& matcher,
                                                         GlobMetadataCache* globMetadataCache)
{
    const auto InputFiles = expandGlobPatterns(
        artifactPatternsForInputs(step), projectRoot, matcher, globMetadataCache);

    std::vector<InputStamp> stamps;
    stamps.reserve(InputFiles.size());
    for (const auto& relativePath : InputFiles)
    {
        const auto Absolute = projectRoot / relativePath;
        std::error_code errorCode;
        if (!std::filesystem::is_regular_file(Absolute, errorCode))
        {
            continue;
        }

        stamps.push_back(InputStamp {
            .path = relativePath,
            .size = std::filesystem::file_size(Absolute, errorCode),
            .modified = std::filesystem::last_write_time(Absolute, errorCode),
        });
    }

    std::ranges::sort(stamps,
                      [](const InputStamp& left, const InputStamp& right)
                      { return left.path < right.path; });
    return stamps;
}

[[nodiscard]] bool inputStampsMatch(const std::vector<InputStamp>& expected,
                                    const std::vector<InputStamp>& actual)
{
    if (expected.size() != actual.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        if (expected.at(index).path != actual.at(index).path ||
            expected.at(index).size != actual.at(index).size ||
            expected.at(index).modified != actual.at(index).modified)
        {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::optional<CacheIndexEntry> readCacheIndex(const std::filesystem::path& indexPath,
                                                            const CacheOptions& options)
{
    if (!std::filesystem::exists(indexPath))
    {
        return std::nullopt;
    }

    const std::string Payload = readCacheFile(indexPath, options);
    std::istringstream stream(Payload);
    CacheIndexEntry entry;
    std::string line;
    while (std::getline(stream, line))
    {
        const auto Equals = line.find('=');
        if (Equals == std::string::npos)
        {
            continue;
        }

        const std::string Field = line.substr(0, Equals);
        const std::string Value = line.substr(Equals + 1);
        if (Field == "key")
        {
            entry.key = Value;
        }
        else if (Field == "command")
        {
            entry.command = Value;
        }
        else if (Field == "config")
        {
            entry.config = Value;
        }
        else if (Field == "version")
        {
            entry.version = Value;
        }
        else if (Field == "duration")
        {
            entry.durationSeconds = std::stod(Value);
        }
        else if (Field == "input")
        {
            const auto FirstSeparator = Value.find('\t');
            const auto SecondSeparator = Value.find('\t', FirstSeparator + 1);
            if (FirstSeparator == std::string::npos || SecondSeparator == std::string::npos)
            {
                continue;
            }

            InputStamp stamp;
            stamp.path = Value.substr(0, FirstSeparator);
            stamp.size = static_cast<std::uintmax_t>(std::stoull(
                Value.substr(FirstSeparator + 1, SecondSeparator - FirstSeparator - 1)));
            stamp.modified =
                std::filesystem::file_time_type(std::filesystem::file_time_type::duration(
                    std::stoll(Value.substr(SecondSeparator + 1))));
            entry.inputs.push_back(std::move(stamp));
        }
        else if (Field == "output")
        {
            entry.outputs.push_back(Value);
        }
    }

    if (entry.key.empty())
    {
        return std::nullopt;
    }

    std::ranges::sort(entry.inputs,
                      [](const InputStamp& left, const InputStamp& right)
                      { return left.path < right.path; });
    return entry;
}

void writeCacheIndex(const std::filesystem::path& indexPath,
                     const CacheIndexEntry& entry,
                     const CacheOptions& options)
{
    std::ostringstream stream;
    stream << "key=" << entry.key << '\n';
    stream << "command=" << entry.command << '\n';
    stream << "config=" << entry.config << '\n';
    stream << "version=" << entry.version << '\n';
    if (entry.durationSeconds > 0.0)
    {
        stream << "duration=" << entry.durationSeconds << '\n';
    }
    for (const auto& stamp : entry.inputs)
    {
        stream << "input=" << stamp.path << '\t' << stamp.size << '\t'
               << stamp.modified.time_since_epoch().count() << '\n';
    }
    for (const auto& output : entry.outputs)
    {
        stream << "output=" << output << '\n';
    }
    writeCacheFile(indexPath, stream.str(), options);
}

}  // namespace beez::core::step_cache_detail
