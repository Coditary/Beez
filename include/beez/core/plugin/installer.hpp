#pragma once

#include "beez/core/util/expected.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace beez::core
{

struct BeezPluginInstallResult
{
    bool installed = false;
    std::string message;
};

[[nodiscard]] std::string formatBeezPluginInstallArg(const std::string& organization,
                                                   const std::string& name,
                                                   const std::string& version);

[[nodiscard]] Expected<std::filesystem::path, std::string>
resolveInstalledBeezPluginScript(const std::optional<std::string>& organization,
                                 const std::string& name,
                                 const std::string& version);

[[nodiscard]] std::optional<std::string>
resolveInstalledBeezPluginOrganization(const std::string& name, const std::string& version);

[[nodiscard]] BeezPluginInstallResult
ensureBeezPluginInstalled(const std::optional<std::string>& organization,
                          const std::string& name,
                          const std::string& version);

}  // namespace beez::core
