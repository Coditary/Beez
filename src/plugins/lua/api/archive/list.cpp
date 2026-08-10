#include "beez/plugin/lua/api/archive/list.hpp"

#include "beez/plugin/lua/api/archive/detail/archive_common.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <filesystem>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::vector<archive_detail::ArchiveEntryInfo>
listEntries(const std::filesystem::path& archivePath)
{
    archive_detail::ArchiveReadHandle handle = archive_detail::openArchiveReader(archivePath);
    std::vector<archive_detail::ArchiveEntryInfo> entries;
    archive_entry* entry = nullptr;

    while (true)
    {
        const int result = archive_read_next_header(handle.reader, &entry);
        if (result == ARCHIVE_EOF)
        {
            break;
        }
        archive_detail::throwOnArchiveError(handle.reader, result, "failed to read archive header");

        archive_detail::ArchiveEntryInfo info = archive_detail::entryInfo(entry);
        if (!info.path.empty())
        {
            entries.push_back(std::move(info));
        }
    }

    return entries;
}

}  // namespace

void bindArchiveList(sol::table& archiveTable,
                     const std::shared_ptr<sol::state>& luaState,
                     const core::Context& context)
{
    archiveTable["list"] = [luaState, &context](const std::string& archivePath) -> sol::table
    {
        const std::filesystem::path resolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        const std::vector<archive_detail::ArchiveEntryInfo> entries = listEntries(resolvedArchive);

        sol::table result = luaState->create_table();
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const archive_detail::ArchiveEntryInfo& entry = entries.at(index);
            sol::table entryTable = luaState->create_table();
            entryTable["path"] = entry.path;
            entryTable["size"] = entry.size;
            entryTable["is_dir"] = entry.isDirectory;
            result.set(static_cast<int>(index + 1), entryTable);
        }
        return result;
    };
}

}  // namespace beez::plugin::lua
