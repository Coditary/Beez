#include "beez/plugin/lua/api/fs/detail/operations.hpp"

#include "beez/plugin/lua/api/detail/path.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::fs_detail
{

std::filesystem::path resolvedPath(const core::Context& context, const std::string& userPath)
{
    return api_detail::resolvePath(context.projectRoot(), userPath);
}

void copyPath(const core::Context& context,
              const std::string& sourcePath,
              const std::string& destinationPath,
              const bool overwrite)
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

}  // namespace beez::plugin::lua::fs_detail
