#pragma once

namespace beez::core
{
class Registry;
}  // namespace beez::core

namespace beez::plugin::lua
{

class ReqpackBeezPluginCatalog;

void validateConfiguredPlugins(const core::Registry& registry,
                               const ReqpackBeezPluginCatalog& catalog);

}  // namespace beez::plugin::lua
