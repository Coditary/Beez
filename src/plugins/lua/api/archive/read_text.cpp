#include "beez/plugin/lua/api/archive/read_text.hpp"

#include "beez/plugin/lua/api/archive/detail/archive_common.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

namespace
{

constexpr std::size_t ReadBufferSize = 64 * 1024;
constexpr std::size_t MaxTextReadBytes = 16 * 1024 * 1024;

[[nodiscard]] std::string readText(const std::filesystem::path& archivePath,
                                   const std::string& entryPath)
{
    archive_detail::ArchiveReadHandle handle = archive_detail::openArchiveReader(archivePath);
    archive_entry* entry = nullptr;

    while (true)
    {
        const int result = archive_read_next_header(handle.reader, &entry);
        if (result == ARCHIVE_EOF)
        {
            break;
        }
        archive_detail::throwOnArchiveError(handle.reader, result, "failed to read archive header");

        if (!archive_detail::entryPathsMatch(archive_entry_pathname(entry), entryPath))
        {
            archive_read_data_skip(handle.reader);
            continue;
        }

        if (archive_entry_filetype(entry) == AE_IFDIR)
        {
            throw std::runtime_error("archive entry is a directory: " + entryPath);
        }

        const auto entrySize = static_cast<std::uint64_t>(archive_entry_size(entry));
        if (entrySize > MaxTextReadBytes)
        {
            throw std::runtime_error("archive entry is too large to read as text: " + entryPath);
        }

        std::string content;
        content.reserve(static_cast<std::size_t>(entrySize));
        std::array<char, ReadBufferSize> buffer {};
        while (true)
        {
            const la_ssize_t read = archive_read_data(handle.reader, buffer.data(), buffer.size());
            if (read == 0)
            {
                break;
            }
            if (read < 0)
            {
                throw std::runtime_error("failed to read archive entry: " +
                                         archive_detail::archiveError(handle.reader));
            }
            content.append(buffer.data(), static_cast<std::size_t>(read));
        }

        return content;
    }

    throw std::runtime_error("archive entry not found: " + entryPath);
}

}  // namespace

void bindArchiveReadText(sol::table& archiveTable, const core::Context& context)
{
    archiveTable["read_text"] = [&context](const std::string& archivePath,
                                           const std::string& fileInArchive) -> std::string
    {
        const std::filesystem::path resolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        return readText(resolvedArchive, fileInArchive);
    };
}

}  // namespace beez::plugin::lua
