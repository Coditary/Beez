#include "beez/plugin/lua/api/archive/extract_file.hpp"

#include "beez/plugin/lua/api/archive/detail/archive_common.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
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

void extractFile(const std::filesystem::path& archivePath,
                 const std::string& entryPath,
                 const std::filesystem::path& destinationPath)
{
    archive_detail::ArchiveReadHandle handle = archive_detail::openArchiveReader(archivePath);
    archive_entry* entry = nullptr;
    bool found = false;

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

        std::filesystem::create_directories(destinationPath.parent_path());
        std::ofstream output(destinationPath, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error("failed to open destination file: " +
                                     destinationPath.string());
        }

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
            output.write(buffer.data(), read);
        }

        found = true;
        break;
    }

    if (!found)
    {
        throw std::runtime_error("archive entry not found: " + entryPath);
    }
}

}  // namespace

void bindArchiveExtractFile(sol::table& archiveTable,
                            const std::shared_ptr<sol::state>& luaState,
                            const core::Context& context)
{
    archiveTable["extract_file"] = [luaState,
                                    &context](const std::string& archivePath,
                                              const std::string& fileInArchive,
                                              const std::string& destinationPath) -> sol::table
    {
        const std::filesystem::path resolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        const std::filesystem::path resolvedDestination =
            api_detail::resolvePath(context.projectRoot(), destinationPath);
        extractFile(resolvedArchive, fileInArchive, resolvedDestination);

        sol::table result = luaState->create_table();
        result["path"] = resolvedDestination.generic_string();
        if (std::filesystem::exists(resolvedDestination))
        {
            result["bytes"] =
                static_cast<std::uint64_t>(std::filesystem::file_size(resolvedDestination));
        }
        return result;
    };
}

}  // namespace beez::plugin::lua
