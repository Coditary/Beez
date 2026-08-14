#include "beez/plugin/lua/lua_dsl.hpp"

#include "beez/plugin/lua/runtime/plugin_config.hpp"

#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/host/plugin_host.hpp"
#include "beez/plugin/lua/dsl/dsl_binder.hpp"
#include "beez/plugin/lua/dsl/registry_validation.hpp"
#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

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
    core::ReqPackManifest reqpackManifest;
    ReqpackBeezPluginCatalog reqpackBeezPlugins;
};

LuaDslLoader::LuaDslLoader() : impl_(std::make_unique<Impl>()) {}

LuaDslLoader::~LuaDslLoader() = default;

bool LuaDslLoader::load(const core::Context& context, core::Registry& registry)
{
    registry.clear();
    clearPluginConfigRegistry();
    try
    {
        impl_->buildSettings = {};
        impl_->reqpackManifest = {};
        impl_->reqpackBeezPlugins = {};
        impl_->luaState = nullptr;
        impl_->luaState = std::make_shared<sol::state>();
        impl_->luaState->open_libraries(sol::lib::base, sol::lib::package);

        registerDsl(impl_->luaState,
                    registry,
                    context,
                    impl_->buildSettings,
                    impl_->reqpackManifest,
                    impl_->reqpackBeezPlugins);

        const auto ScriptPath = context.buildScriptPath().string();
        impl_->luaState->script_file(ScriptPath);
        validateLoadedRegistry(registry, impl_->reqpackBeezPlugins);
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

const core::ReqPackManifest& LuaDslLoader::reqpackManifest() const
{
    return impl_->reqpackManifest;
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
