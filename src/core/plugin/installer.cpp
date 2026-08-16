#include "beez/core/plugin/installer.hpp"

#include "beez/core/plugin/paths.hpp"
#include "beez/core/reqpack/installer.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/plugin/lua/dsl/plugin_loader.hpp"

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

namespace beez::core
{

namespace
{

[[nodiscard]] std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (const char character : value)
    {
        if (character == '\'')
        {
            quoted += "'\\''";
        }
        else
        {
            quoted += character;
        }
    }
    quoted += '\'';
    return quoted;
}

}  // namespace

std::string formatBeezPluginInstallArg(const std::string& organization,
                                       const std::string& name,
                                       const std::string& version,
                                       const std::optional<std::string>& source)
{
    static_cast<void>(source);
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
                                                  const std::string& version,
                                                  const std::optional<std::string>& source)
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

    if (version.empty())
    {
        return {.installed = false, .message = "beez plugin install requires a version pin"};
    }

    if (!isRqpAvailable())
    {
        return {.installed = false,
                .message = "ReqPack (rqp) is required to install beez plugins. Install it with:\n"
                           "  curl -fsSL https://raw.githubusercontent.com/Coditary/ReqPack/main/"
                           "install.sh | sh\n"};
    }

    std::ostringstream command;
    if (source.has_value() && !source->empty())
    {
        command << "BEEZ_PLUGIN_SOURCE=" << shellQuote(*source) << ' ';
    }
    command << "rqp --json install " << formatBeezPluginInstallArg(*organization, name, version);

    const auto CommandResult = executeRqpCommand(command.str());
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

BeezPluginInstallResult ensureReqpackBeezPluginsInstalled(
    const std::vector<plugin::lua::BeezPluginRef>& plugins)
{
    for (const auto& pluginRef : plugins)
    {
        if (pluginRef.isLocal())
        {
            continue;
        }

        if (!pluginRef.version.has_value() || pluginRef.version->empty())
        {
            return {.installed = false,
                    .message = "remote reqpack.beez plugin '" + pluginRef.organization + '/' +
                               pluginRef.name + "' requires a version pin"};
        }

        const auto Result = ensureBeezPluginInstalled(pluginRef.organization,
                                                      pluginRef.name,
                                                      *pluginRef.version,
                                                      pluginRef.source);
        if (!Result.message.empty() &&
            (Result.message.find("failed to install") != std::string::npos ||
             Result.message.find("is not available after install") != std::string::npos ||
             Result.message.find("requires a version pin") != std::string::npos ||
             Result.message.find("ReqPack (rqp) is required") != std::string::npos))
        {
            return Result;
        }
    }

    return {.installed = true, .message = {}};
}

}  // namespace beez::core
