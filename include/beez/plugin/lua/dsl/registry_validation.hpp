#pragma once

namespace beez::core
{
class Registry;
}  // namespace beez::core

namespace beez::plugin::lua
{

class ReqpackBeezPluginCatalog;

void validateLoadedRegistry(core::Registry& registry,
                            const ReqpackBeezPluginCatalog& reqpackBeezPlugins);

}  // namespace beez::plugin::lua
