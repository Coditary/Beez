#include "beez/plugin/lua/lua_dsl.hpp"

#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/plugin/host/plugin_host.hpp"
#include "beez/plugin/lua/dsl/dsl_binder.hpp"
#include "beez/plugin/lua/dsl/registry_validation.hpp"

#include <iostream>
#include <memory>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

struct LuaDslLoader::Impl
{
    std::shared_ptr<sol::state> luaState;
    core::BeezSettings buildSettings;
};

LuaDslLoader::LuaDslLoader() : impl_(std::make_unique<Impl>()) {}

LuaDslLoader::~LuaDslLoader() = default;

bool LuaDslLoader::load(const core::Context& context, core::Registry& registry)
{
    registry.clear();
    try
    {
        impl_->buildSettings = {};
        impl_->luaState = nullptr;
        impl_->luaState = std::make_shared<sol::state>();
        impl_->luaState->open_libraries(sol::lib::base, sol::lib::package);

        registerDsl(impl_->luaState, registry, context, impl_->buildSettings);

        const auto ScriptPath = context.buildScriptPath().string();
        impl_->luaState->script_file(ScriptPath);
        validateLoadedRegistry(registry);
        return true;
    }
    catch (const sol::error& error)
    {
        std::cerr << "Lua error: " << error.what() << '\n';
        impl_->luaState = nullptr;
        registry.clear();
        return false;
    }
    catch (const std::exception& error)
    {
        std::cerr << "DSL error: " << error.what() << '\n';
        impl_->luaState = nullptr;
        registry.clear();
        return false;
    }
}

const core::BeezSettings& LuaDslLoader::buildSettings() const
{
    return impl_->buildSettings;
}

void LuaDslLoader::setGcThroughputMode(bool enable)
{
    if (impl_->luaState == nullptr)
    {
        return;
    }

    sol::state_view view(*impl_->luaState);
    if (enable)
    {
        view.stop_gc();
        return;
    }

    view.restart_gc();
    view.collect_garbage();
}

void LuaDslLoader::releaseState()
{
    impl_->luaState = nullptr;
}

std::string LuaDslPlugin::name() const
{
    return "lua_dsl";
}

void LuaDslPlugin::registerCapabilities(PluginHost& host)
{
    host.setDslLoader(std::make_unique<LuaDslLoader>());
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
