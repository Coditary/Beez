#include "beez/core/config/paths/config_paths.hpp"

#include <cstdlib>
#include <filesystem>

namespace beez::core
{

std::filesystem::path beezConfigDirectory()
{
    // NOLINTBEGIN(concurrency-mt-unsafe) -- configuration path lookup at startup
    if (const char* configHome = std::getenv("XDG_CONFIG_HOME"); configHome != nullptr)
    {
        return std::filesystem::path(configHome) / "beez";
    }

    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        return std::filesystem::path(home) / ".config" / "beez";
    }
    // NOLINTEND(concurrency-mt-unsafe)

    return {};
}

std::filesystem::path globalBeezConfigPath()
{
    const auto ConfigDirectory = beezConfigDirectory();
    if (ConfigDirectory.empty())
    {
        return {};
    }

    return ConfigDirectory / "config.lua";
}

std::filesystem::path globalBuildScriptPath()
{
    const auto ConfigDirectory = beezConfigDirectory();
    if (ConfigDirectory.empty())
    {
        return {};
    }

    return ConfigDirectory / "global" / "build.lua";
}

std::filesystem::path profileBeezConfigPath(const std::string& profileName)
{
    const auto ConfigDirectory = beezConfigDirectory();
    if (ConfigDirectory.empty())
    {
        return {};
    }

    return ConfigDirectory / "profiles" / (profileName + ".lua");
}

}  // namespace beez::core
