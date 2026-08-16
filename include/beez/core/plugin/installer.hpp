#pragma once

#include "beez/core/util/expected.hpp"
#include "beez/plugin/lua/beez_plugin_ref.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

struct BeezPluginInstallResult
{
    bool installed = false;
    std::string message;
};

[[nodiscard]] std::string formatBeezPluginInstallArg(const std::string& organization,
                                                     const std::string& name,
                                                     const std::string& version,
                                                     const std::optional<std::string>& source = std::nullopt);

[[nodiscard]] Expected<std::filesystem::path, std::string>
resolveInstalledBeezPluginScript(const std::optional<std::string>& organization,
                                 const std::string& name,
                                 const std::string& version);

[[nodiscard]] std::optional<std::string>
resolveInstalledBeezPluginOrganization(const std::string& name, const std::string& version);

[[nodiscard]] BeezPluginInstallResult
ensureBeezPluginInstalled(const std::optional<std::string>& organization,
                          const std::string& name,
                          const std::string& version,
                          const std::optional<std::string>& source = std::nullopt);

[[nodiscard]] BeezPluginInstallResult ensureReqpackBeezPluginsInstalled(
    const std::vector<plugin::lua::BeezPluginRef>& plugins);

}  // namespace beez::core
