#include "beez/plugin/lua/dsl/plugin_config_validation.hpp"

#include "beez/core/registry/registry.hpp"
#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::pair<std::string, std::string> splitQualifiedPluginName(const std::string& name)
{
    const auto SlashPosition = name.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0 ||
        SlashPosition == name.size() - 1)
    {
        throw std::runtime_error("plugin name '" + name +
                                 "' must use the form 'organization/plugin'");
    }

    return {name.substr(0, SlashPosition), name.substr(SlashPosition + 1)};
}

}  // namespace

void validateConfiguredPlugins(const core::Registry& registry,
                               const ReqpackBeezPluginCatalog& catalog)
{
    for (const auto& [pluginKey, config] : registry.configuredPluginConfigs())
    {
        (void)config;
        const auto [Organization, Plugin] = splitQualifiedPluginName(pluginKey);
        if (!catalog.find(Organization, Plugin).has_value())
        {
            throw std::runtime_error("configure_plugin references plugin '" + pluginKey +
                                     "' which is not declared in reqpack.beez");
        }

        if (!registry.hasPluginSteps(Organization, Plugin))
        {
            throw std::runtime_error("configure_plugin references plugin '" + pluginKey +
                                     "' which has no registered steps");
        }
    }
}

}  // namespace beez::plugin::lua
