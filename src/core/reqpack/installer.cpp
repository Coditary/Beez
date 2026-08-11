#include "beez/core/reqpack/installer.hpp"

#include "beez/core/reqpack/cache.hpp"
#include "beez/core/reqpack/format.hpp"
#include "beez/core/reqpack/types.hpp"
#include "beez/core/runtime/context.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

constexpr std::size_t CaptureBufferSize = 256;

constexpr const char* ReqPackInstallHint =
    "ReqPack (rqp) is required to install dependencies. Install it with:\n"
    "  curl -fsSL https://raw.githubusercontent.com/Coditary/ReqPack/main/install.sh | sh\n";

[[nodiscard]] ReqPackManifest
manifestFromUncached(const std::map<std::string, std::vector<ReqPackPackage>>& uncached)
{
    ReqPackManifest manifest;
    for (const auto& [plugin, packages] : uncached)
    {
        manifest.plugins.emplace(plugin, packages);
    }
    return manifest;
}

[[nodiscard]] bool pluginFullySucceeded(const std::string& plugin,
                                        const ReqPackInstallResponse& response)
{
    const auto Found = response.plugins.find(plugin);
    if (Found == response.plugins.end())
    {
        return response.succeeded();
    }

    return std::ranges::all_of(
        Found->second, [](const ReqPackPackageResult& package) { return !package.failed(); });
}

[[nodiscard]] std::string buildExitCodeMessage(int exitCode, std::string_view output)
{
    std::ostringstream stream;
    stream << "rqp install failed with exit code " << exitCode;
    if (!output.empty())
    {
        stream << "\n" << output;
    }
    return stream.str();
}

}  // namespace

bool isRqpAvailable()
{
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe)
    const int Status = std::system("command -v rqp >/dev/null 2>&1");
    return Status == 0;
}

// NOLINTBEGIN(misc-include-cleaner)
#include <sys/wait.h>

RqpCommandResult executeRqpCommand(const std::string& command)
{
    const std::string ShellCommand = "(" + command + ") 2>&1";
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe,misc-include-cleaner)
    FILE* pipe = popen(ShellCommand.c_str(), "r");
    if (pipe == nullptr)
    {
        return {.exitCode = -1};
    }

    std::string output;
    std::array<char, CaptureBufferSize> buffer {};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }

    // NOLINTNEXTLINE(misc-include-cleaner)
    const int Status = pclose(pipe);
    if (Status == -1)
    {
        return {.exitCode = -1, .output = std::move(output)};
    }

    if (WIFEXITED(Status))
    {
        return {.exitCode = WEXITSTATUS(Status), .output = output};
    }

    return {.exitCode = -1, .output = std::move(output)};
}
// NOLINTEND(misc-include-cleaner)

ReqPackInstallResult installReqPackDependencies(
    const ReqPackManifest& manifest,
    const Context& context,
    const ReqPackInstallOptions& options,
    const std::function<RqpCommandResult(const std::string& command)>& execute)
{
    if (manifest.empty())
    {
        return {.skipped = true, .success = true};
    }

    std::map<std::string, std::vector<ReqPackPackage>> toInstall;
    if (options.forceInstall)
    {
        toInstall = manifest.plugins;
    }
    else
    {
        toInstall = filterUncachedPlugins(manifest, context.projectRoot());
    }

    if (toInstall.empty())
    {
        return {.skipped = true, .success = true, .message = "reqpack dependencies are up to date"};
    }

    if (!isRqpAvailable())
    {
        return {.skipped = false, .success = false, .message = std::string(ReqPackInstallHint)};
    }

    const auto InstallManifest = manifestFromUncached(toInstall);
    const auto Args = buildInstallArgs(InstallManifest);
    if (Args.empty())
    {
        return {.skipped = true, .success = true};
    }

    const auto Command = buildInstallCommand(Args, options.dryRun);
    if (options.dryRun)
    {
        return {.skipped = false, .success = true, .message = Command};
    }

    const auto CommandResult = execute(Command);

    ReqPackInstallResponse response;
    bool parsedResponse = false;
    try
    {
        if (!CommandResult.output.empty())
        {
            response = parseRqpJsonResponse(CommandResult.output);
            parsedResponse = true;
        }
    }
    catch (const std::exception& /*error*/)
    {
        parsedResponse = false;
    }

    if (parsedResponse && !response.succeeded())
    {
        return {.skipped = false,
                .success = false,
                .message = formatRqpInstallErrors(response),
                .response = response};
    }

    if (!parsedResponse)
    {
        if (CommandResult.exitCode != 0)
        {
            return {.skipped = false,
                    .success = false,
                    .message = buildExitCodeMessage(CommandResult.exitCode, CommandResult.output)};
        }

        return {.skipped = false, .success = false, .message = "failed to parse rqp json output"};
    }

    if (CommandResult.exitCode != 0)
    {
        return {.skipped = false,
                .success = false,
                .message = buildExitCodeMessage(CommandResult.exitCode, CommandResult.output),
                .response = response};
    }

    for (const auto& [plugin, packages] : toInstall)
    {
        if (pluginFullySucceeded(plugin, response))
        {
            updatePluginCache(plugin, packages, context.projectRoot());
        }
    }

    return {.skipped = false, .success = true, .response = std::move(response)};
}

}  // namespace beez::core
