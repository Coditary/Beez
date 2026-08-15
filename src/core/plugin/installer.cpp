#include "beez/core/plugin/installer.hpp"

#include "beez/core/plugin/paths.hpp"
#include "beez/core/reqpack/installer.hpp"
#include "beez/core/util/expected.hpp"

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

namespace beez::core
{

std::string formatBeezPluginInstallArg(const std::string& organization,
                                       const std::string& name,
                                       const std::string& version)
{
    return "beez:" + organization + '/' + name + '@' + version;
}

Expected<std::filesystem::path, std::string>
resolveInstalledBeezPluginScript(const std::optional<std::string>& organization,
                                 const std::string& name,
                                 const std::string& version)
{
    if (organization.has_value())
    {
        if (const auto ScriptPath = findPluginScript(*organization, name, version))
        {
            return *ScriptPath;
        }

        return "installed beez plugin '" + *organization + '/' + name + '@' + version +
               "' was not found";
    }

    if (const auto ScriptPath = findPluginScript(name, version))
    {
        return *ScriptPath;
    }

    return "installed beez plugin '" + name + '@' + version + "' was not found";
}

std::optional<std::string> resolveInstalledBeezPluginOrganization(const std::string& name,
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

    for (const auto& organizationEntry : std::filesystem::directory_iterator(PluginRoot, errorCode))
    {
        if (errorCode || !organizationEntry.is_directory())
        {
            continue;
        }

        const auto ScriptPath = organizationEntry.path() / name / version / "beez_plugin.lua";
        if (std::filesystem::is_regular_file(ScriptPath, errorCode) && !errorCode)
        {
            return organizationEntry.path().filename().string();
        }
    }

    return std::nullopt;
}

BeezPluginInstallResult ensureBeezPluginInstalled(const std::optional<std::string>& organization,
                                                  const std::string& name,
                                                  const std::string& version)
{
    if (resolveInstalledBeezPluginScript(organization, name, version).hasValue())
    {
        return {.installed = false, .message = {}};
    }

    if (!organization.has_value())
    {
        return {.installed = false,
                .message = "beez plugin version pin requires organization/plugin reference"};
    }

    if (!isRqpAvailable())
    {
        return {.installed = false,
                .message = "ReqPack (rqp) is required to install beez plugins. Install it with:\n"
                           "  curl -fsSL https://raw.githubusercontent.com/Coditary/ReqPack/main/"
                           "install.sh | sh\n"};
    }

    const auto InstallArg = formatBeezPluginInstallArg(*organization, name, version);
    const auto CommandResult = executeRqpCommand("rqp --json install " + InstallArg);
    if (CommandResult.exitCode != 0)
    {
        std::ostringstream stream;
        stream << "failed to install beez plugin '" << *organization << '/' << name << '@'
               << version << "' (exit code " << CommandResult.exitCode << ')';
        if (!CommandResult.output.empty())
        {
            stream << "\n" << CommandResult.output;
        }
        return {.installed = false, .message = stream.str()};
    }

    if (!resolveInstalledBeezPluginScript(organization, name, version).hasValue())
    {
        return {.installed = false,
                .message = "beez plugin '" + *organization + '/' + name + '@' + version +
                           "' is not available after install"};
    }

    return {.installed = true, .message = {}};
}

}  // namespace beez::core
