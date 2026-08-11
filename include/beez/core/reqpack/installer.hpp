#pragma once

#include "beez/core/reqpack/types.hpp"
#include "beez/core/runtime/context.hpp"

#include <functional>
#include <string>

namespace beez::core
{

struct ReqPackInstallOptions
{
    bool dryRun = false;
    bool forceInstall = false;
};

struct RqpCommandResult
{
    int exitCode = 0;
    std::string output;
};

struct ReqPackInstallResult
{
    bool skipped = false;
    bool success = true;
    std::string message;
    ReqPackInstallResponse response;
};

[[nodiscard]] bool isRqpAvailable();

[[nodiscard]] RqpCommandResult executeRqpCommand(const std::string& command);

[[nodiscard]] ReqPackInstallResult installReqPackDependencies(
    const ReqPackManifest& manifest,
    const Context& context,
    const ReqPackInstallOptions& options = {},
    const std::function<RqpCommandResult(const std::string& command)>& execute = executeRqpCommand);

}  // namespace beez::core
