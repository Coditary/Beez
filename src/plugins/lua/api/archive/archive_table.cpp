#include "beez/plugin/lua/api/archive/archive_table.hpp"

#include "beez/plugin/lua/api/archive/detail/archive_ops.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::optional<std::string>
parseFormatOption(const sol::optional<sol::table>& optionsTable)
{
    if (!optionsTable.has_value())
    {
        return std::nullopt;
    }

    const sol::object FormatValue = optionsTable->get<sol::object>("format");
    if (!FormatValue.valid() || FormatValue.is<sol::lua_nil_t>())
    {
        return std::nullopt;
    }

    if (!FormatValue.is<std::string>())
    {
        throw std::runtime_error("archive compress option 'format' must be a string");
    }

    return FormatValue.as<std::string>();
}

}  // namespace

sol::table bindArchive(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table archiveTable = luaState->create_table();

    archiveTable["extract"] =
        [luaState, &context](const std::string& archivePath, const std::string& destinationDir) -> sol::table
    {
        const std::filesystem::path ResolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        const std::filesystem::path ResolvedDestination =
            api_detail::resolvePath(context.projectRoot(), destinationDir);
        const std::size_t Entries =
            archive_detail::extractAll(ResolvedArchive, ResolvedDestination);

        sol::table result = luaState->create_table();
        result["entries"] = Entries;
        result["dest"] = ResolvedDestination.generic_string();
        return result;
    };

    archiveTable["compress"] =
        [luaState, &context](const std::string& sourcePath,
                   const std::string& archivePath,
                   const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        const std::filesystem::path ResolvedSource =
            api_detail::resolvePath(context.projectRoot(), sourcePath);
        const std::filesystem::path ResolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        archive_detail::compress(ResolvedSource, ResolvedArchive, parseFormatOption(optionsTable));

        sol::table result = luaState->create_table();
        result["path"] = ResolvedArchive.generic_string();
        if (std::filesystem::exists(ResolvedArchive))
        {
            result["bytes"] = static_cast<std::uint64_t>(std::filesystem::file_size(ResolvedArchive));
        }
        return result;
    };

    archiveTable["list"] =
        [luaState, &context](const std::string& archivePath) -> sol::table
    {
        const std::filesystem::path ResolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        const std::vector<archive_detail::ArchiveEntryInfo> Entries =
            archive_detail::listEntries(ResolvedArchive);

        sol::table result = luaState->create_table();
        for (std::size_t index = 0; index < Entries.size(); ++index)
        {
            const archive_detail::ArchiveEntryInfo& entry = Entries.at(index);
            sol::table entryTable = luaState->create_table();
            entryTable["path"] = entry.path;
            entryTable["size"] = entry.size;
            entryTable["is_dir"] = entry.isDirectory;
            result.set(static_cast<int>(index + 1), entryTable);
        }
        return result;
    };

    archiveTable["extract_file"] =
        [luaState, &context](const std::string& archivePath,
                   const std::string& fileInArchive,
                   const std::string& destinationPath) -> sol::table
    {
        const std::filesystem::path ResolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        const std::filesystem::path ResolvedDestination =
            api_detail::resolvePath(context.projectRoot(), destinationPath);
        archive_detail::extractFile(ResolvedArchive, fileInArchive, ResolvedDestination);

        sol::table result = luaState->create_table();
        result["path"] = ResolvedDestination.generic_string();
        if (std::filesystem::exists(ResolvedDestination))
        {
            result["bytes"] = static_cast<std::uint64_t>(std::filesystem::file_size(ResolvedDestination));
        }
        return result;
    };

    archiveTable["read_text"] =
        [&context](const std::string& archivePath, const std::string& fileInArchive) -> std::string
    {
        const std::filesystem::path ResolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        return archive_detail::readText(ResolvedArchive, fileInArchive);
    };

    return archiveTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
