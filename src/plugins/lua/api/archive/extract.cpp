#include "beez/plugin/lua/api/archive/extract.hpp"

#include "beez/plugin/lua/api/archive/detail/archive_common.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <filesystem>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,misc-const-correctness,readability-identifier-naming)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::size_t extractAll(const std::filesystem::path& archivePath,
                                     const std::filesystem::path& destinationDir)
{
    std::filesystem::create_directories(destinationDir);
    archive_detail::ArchiveReadHandle handle = archive_detail::openArchiveReader(archivePath);

    const std::filesystem::path previousCwd = std::filesystem::current_path();
    std::filesystem::current_path(destinationDir);

    std::size_t extractedCount = 0;
    archive_entry* entry = nullptr;
    try
    {
        while (true)
        {
            const int result = archive_read_next_header(handle.reader, &entry);
            if (result == ARCHIVE_EOF)
            {
                break;
            }
            archive_detail::throwOnArchiveError(
                handle.reader, result, "failed to read archive header");

            const std::string relativePath =
                archive_detail::normalizeEntryPath(archive_entry_pathname(entry));
            archive_entry_set_pathname(entry, relativePath.c_str());
            archive_detail::throwOnArchiveError(
                handle.reader,
                archive_read_extract(handle.reader, entry, archive_detail::ExtractFlags),
                "failed to extract archive entry");
            ++extractedCount;
        }
    }
    catch (...)
    {
        std::filesystem::current_path(previousCwd);
        throw;
    }

    std::filesystem::current_path(previousCwd);
    return extractedCount;
}

}  // namespace

void bindArchiveExtract(sol::table& archiveTable,
                        const std::shared_ptr<sol::state>& luaState,
                        const core::Context& context)
{
    archiveTable["extract"] = [luaState, &context](const std::string& archivePath,
                                                   const std::string& destinationDir) -> sol::table
    {
        const std::filesystem::path resolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        const std::filesystem::path resolvedDestination =
            api_detail::resolvePath(context.projectRoot(), destinationDir);
        const std::size_t entries = extractAll(resolvedArchive, resolvedDestination);

        sol::table result = luaState->create_table();
        result["entries"] = entries;
        result["dest"] = resolvedDestination.generic_string();
        return result;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,misc-const-correctness,readability-identifier-naming)
