#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/core/settings.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <filesystem>
#include <optional>

namespace beez::plugin::lua
{
class LuaDslLoader;
}

namespace beez::cli
{

struct LoadedProject
{
    core::Context context;
    core::BeezSettings globalSettings;
    core::BeezSettings projectSettings;
    core::BeezSettings settings;
    std::filesystem::path configPath;
    core::Registry registry;
    plugin::PluginHost pluginHost;
    plugin::lua::LuaDslLoader* luaLoader = nullptr;
};

void loadGlobalSettings(LoadedProject& project);

[[nodiscard]] std::optional<int> loadBuildScript(LoadedProject& project, bool silentRun);

void mergeProjectSettings(LoadedProject& project, const ParsedOptions& options);

[[nodiscard]] int runWithOrchestrator(LoadedProject& project, const ParsedOptions& options);

}  // namespace beez::cli
