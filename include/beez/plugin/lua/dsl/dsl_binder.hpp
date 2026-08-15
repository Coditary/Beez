#pragma once

#include "beez/core/config/settings/settings.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/reqpack/types.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include <memory>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void registerDsl(const std::shared_ptr<sol::state>& luaState,
                 core::Registry& registry,
                 const core::Context& context,
                 core::BeezSettings& buildSettings,
                 core::ReqPackManifest& reqpackManifest,
                 ReqpackBeezPluginCatalog& reqpackBeezPlugins);

}  // namespace beez::plugin::lua
