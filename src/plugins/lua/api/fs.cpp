#include "beez/plugin/lua/api/fs.hpp"

#include "beez/plugin/lua/api/detail/glob.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::filesystem::path resolvedPath(const core::Context& context,
                                                 const std::string& userPath)
{
    return api_detail::resolvePath(context.projectRoot(), userPath);
}

void copyPath(const core::Context& context,
              const std::string& sourcePath,
              const std::string& destinationPath,
              bool overwrite)
{
    const auto Source = resolvedPath(context, sourcePath);
    const auto Destination = resolvedPath(context, destinationPath);

    if (!std::filesystem::exists(Source))
    {
        throw std::runtime_error("beez.fs.copy: source does not exist: " + sourcePath);
    }

    if (std::filesystem::exists(Destination) && !overwrite)
    {
        throw std::runtime_error("beez.fs.copy: destination already exists: " + destinationPath);
    }

    std::filesystem::copy_options options = std::filesystem::copy_options::none;
    if (overwrite)
    {
        options |= std::filesystem::copy_options::overwrite_existing;
    }

    if (std::filesystem::is_directory(Source))
    {
        options |= std::filesystem::copy_options::recursive;
    }

    std::filesystem::copy(Source, Destination, options);
}

}  // namespace

sol::table bindFs(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table fsTable = luaState->create_table();

    fsTable["glob"] = [luaState, &context](const std::string& pattern) -> sol::table
    { return api_detail::globPatternsToTable(luaState, {pattern}, context.projectRoot()); };

    fsTable["exists"] = [&context](const std::string& path) -> bool
    { return std::filesystem::exists(resolvedPath(context, path)); };

    fsTable["copy"] =
        [&context](const std::string& sourcePath,
                   const std::string& destinationPath,
                   sol::optional<bool> overwrite) -> void
    { copyPath(context, sourcePath, destinationPath, overwrite.value_or(false)); };

    fsTable["remove"] = [&context](const std::string& path) -> bool
    { return std::filesystem::remove_all(resolvedPath(context, path)) > 0; };

    fsTable["join"] = [](sol::variadic_args segments) -> std::string
    {
        std::vector<std::string> parts;
        parts.reserve(segments.size());
        for (const sol::stack_proxy& segment : segments)
        {
            if (!segment.is<std::string>())
            {
                throw std::runtime_error("beez.fs.join: all arguments must be strings");
            }

            parts.push_back(segment.as<std::string>());
        }

        return api_detail::joinPathSegments(parts);
    };

    return fsTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
