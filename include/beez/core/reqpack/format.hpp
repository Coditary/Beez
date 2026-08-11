#pragma once

#include "beez/core/reqpack/types.hpp"

#include <string>
#include <vector>

namespace beez::core
{

[[nodiscard]] std::string formatInstallArg(const std::string& plugin,
                                           const ReqPackPackage& package);

[[nodiscard]] std::vector<std::string> buildInstallArgs(const ReqPackManifest& manifest);

[[nodiscard]] std::string buildInstallCommand(const std::vector<std::string>& args,
                                              bool dryRun = false);

[[nodiscard]] ReqPackInstallResponse parseRqpJsonResponse(const std::string& json);

[[nodiscard]] std::string formatRqpInstallErrors(const ReqPackInstallResponse& response);

}  // namespace beez::core
