#pragma once

#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/registry/registry.hpp"
#include "beez/core/run_options.hpp"
#include "beez/plugin/host/plugin_host.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"
#include "beez/plugin/shell/shell_executor.hpp"

#include <filesystem>
#include <memory>

namespace beez::test
{

class BeezRuntime
{
  public:
    explicit BeezRuntime(const std::filesystem::path& projectRoot) : context_(projectRoot)
    {
        pluginHost_.addPlugin(std::make_unique<beez::plugin::lua::LuaDslPlugin>());
        pluginHost_.addPlugin(std::make_unique<beez::plugin::shell::ShellPlugin>());
        pluginHost_.initialize(registry_, context_);
    }

    [[nodiscard]] beez::core::Orchestrator orchestrator(const beez::core::RunOptions& options = {})
    {
        return {registry_, context_, pluginHost_, options};
    }

    [[nodiscard]] const beez::core::Registry& registry() const
    {
        return registry_;
    }

    [[nodiscard]] const beez::core::Context& context() const
    {
        return context_;
    }

  private:
    beez::core::Context context_;
    beez::core::Registry registry_;
    beez::plugin::PluginHost pluginHost_;
};

}  // namespace beez::test
