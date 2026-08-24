#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <filesystem>
#include <optional>

namespace beez::plugin::lua
{
class LuaDslLoader;
}

namespace beez::cli
{

enum class ScriptSource
{
    Auto,
    Bridge,
    Global,
};

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

[[nodiscard]] std::optional<int> loadBuildScript(LoadedProject& project,
                                                 bool silentRun,
                                                 bool validateRegistry = true,
                                                 ScriptSource source = ScriptSource::Auto);

void mergeProjectSettings(LoadedProject& project, const ParsedOptions& options);

[[nodiscard]] int runWithOrchestrator(LoadedProject& project, const ParsedOptions& options);

}  // namespace beez::cli
