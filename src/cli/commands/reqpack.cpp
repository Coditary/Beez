#include "beez/cli/commands/reqpack.hpp"

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/cli/session.hpp"
#include "beez/core/plugin/installer.hpp"
#include "beez/core/reqpack/installer.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include <iostream>
#include <optional>

namespace beez::cli
{

namespace
{

[[nodiscard]] core::ReqPackInstallOptions makeInstallOptions(const ParsedOptions& options)
{
    return {.dryRun = options.dryRun, .forceInstall = options.installDependencies};
}

[[nodiscard]] std::optional<int> handleInstallResult(const core::ReqPackInstallResult& result,
                                                     bool silentRun)
{
    if (!result.message.empty() && !silentRun)
    {
        if (result.success)
        {
            std::cout << result.message << '\n';
        }
        else
        {
            std::cerr << result.message << '\n';
        }
    }

    if (!result.success)
    {
        return 1;
    }

    return std::nullopt;
}

}  // namespace

std::optional<int> runReqPackInstallCommand(const LoadedProject& project,
                                            const ParsedOptions& options)
{
    const auto BeezPluginResult =
        core::ensureReqpackBeezPluginsInstalled(project.luaLoader->reqpackBeezPlugins().plugins());
    // A non-empty message always reports a failure (see ensureReqpackBeezPluginsInstalled).
    if (!BeezPluginResult.message.empty())
    {
        if (!options.silent)
        {
            std::cerr << BeezPluginResult.message << '\n';
        }
        return 1;
    }

    const auto Result = core::installReqPackDependencies(
        project.luaLoader->reqpackManifest(), project.context, makeInstallOptions(options));
    if (const auto ExitCode = handleInstallResult(Result, options.silent))
    {
        return ExitCode;
    }

    return 0;
}

std::optional<int> ensureReqPackDependencies(const LoadedProject& project,
                                             const ParsedOptions& options)
{
    if (project.luaLoader == nullptr)
    {
        return std::nullopt;
    }

    const auto& manifest = project.luaLoader->reqpackManifest();
    if (manifest.empty())
    {
        return std::nullopt;
    }

    const auto Result =
        core::installReqPackDependencies(manifest, project.context, {.dryRun = options.dryRun});
    if (!Result.success)
    {
        if (!options.silent)
        {
            std::cerr << Result.message << '\n';
        }
        return 1;
    }

    return std::nullopt;
}

}  // namespace beez::cli
