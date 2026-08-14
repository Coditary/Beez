#include "beez/plugin/lua/dsl/workflows_parser.hpp"

#include "beez/core/registry/workflow_reference.hpp"
#include "beez/plugin/lua/dsl/workflow_parser.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

void validatePluginWorkflowReference(const std::string& reference,
                                     const ReqpackBeezPluginCatalog* reqpackBeezPlugins)
{
    const core::PluginWorkflowRef ParsedReference = core::parsePluginWorkflowReference(reference);
    if (reqpackBeezPlugins != nullptr && !reqpackBeezPlugins->empty() &&
        !reqpackBeezPlugins->find(ParsedReference.organization, ParsedReference.plugin).has_value())
    {
        throw std::runtime_error("workflows reference '" + reference + "' references plugin '" +
                                 ParsedReference.organization + '/' + ParsedReference.plugin +
                                 "' which is not declared in reqpack.beez");
    }
}

}  // namespace

void parseWorkflowsTable(const sol::table& workflowsTable,
                         core::Registry& registry,
                         const ReqpackBeezPluginCatalog* reqpackBeezPlugins)
{
    workflowsTable.for_each(
        [&registry, reqpackBeezPlugins](const sol::object& key, const sol::object& value)
        {
            if (!key.is<std::string>() || key.as<std::string>().empty())
            {
                throw std::runtime_error("workflows keys must be non-empty workflow name strings");
            }

            const std::string LocalName = key.as<std::string>();

            if (value.is<std::string>())
            {
                const std::string PluginWorkflowReference = value.as<std::string>();
                validatePluginWorkflowReference(PluginWorkflowReference, reqpackBeezPlugins);
                registry.registerWorkflowFromPluginReference(LocalName, PluginWorkflowReference);
                return;
            }

            if (value.is<sol::table>())
            {
                registry.registerWorkflow(parseWorkflow(LocalName, value.as<sol::table>()));
                return;
            }

            throw std::runtime_error("workflows entry '" + LocalName +
                                     "' must be a plugin reference string or workflow table");
        });
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
