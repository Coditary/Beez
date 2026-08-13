#include "beez/core/plugin/paths.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace beez::core
{

std::filesystem::path beezCacheDirectory()
{
    // NOLINTBEGIN(concurrency-mt-unsafe) -- cache path lookup at startup
    if (const char* cacheHome = std::getenv("XDG_CACHE_HOME"); cacheHome != nullptr)
    {
        return std::filesystem::path(cacheHome) / "beez";
    }

    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        return std::filesystem::path(home) / ".cache" / "beez";
    }
    // NOLINTEND(concurrency-mt-unsafe)

    return {};
}

std::filesystem::path beezPluginRoot()
{
    const auto CacheDirectory = beezCacheDirectory();
    if (CacheDirectory.empty())
    {
        return {};
    }

    return CacheDirectory / "plugins";
}

std::optional<std::filesystem::path> findPluginScript(const std::string& name,
                                                      const std::string& version)
{
    const auto PluginRoot = beezPluginRoot();
    if (PluginRoot.empty())
    {
        return std::nullopt;
    }

    std::error_code errorCode;
    if (!std::filesystem::exists(PluginRoot, errorCode))
    {
        return std::nullopt;
    }

    for (const auto& organizationEntry :
         std::filesystem::directory_iterator(PluginRoot, errorCode))
    {
        if (errorCode || !organizationEntry.is_directory())
        {
            continue;
        }

        const auto ScriptPath =
            organizationEntry.path() / name / version / "beez_plugin.lua";
        if (std::filesystem::is_regular_file(ScriptPath, errorCode) && !errorCode)
        {
            return ScriptPath;
        }
    }

    return std::nullopt;
}

}  // namespace beez::core
