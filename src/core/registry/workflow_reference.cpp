#include "beez/core/registry/workflow_reference.hpp"

#include <stdexcept>
#include <string>

namespace beez::core
{

std::string formatPluginWorkflowKey(const std::string& organization,
                                    const std::string& plugin,
                                    const std::string& workflowName)
{
    return organization + '/' + plugin + ':' + workflowName;
}

PluginWorkflowRef parsePluginWorkflowReference(const std::string& reference)
{
    const auto SlashPosition = reference.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0U)
    {
        throw std::runtime_error("plugin workflow reference '" + reference +
                                 "' must use the form 'organization/plugin:workflow'");
    }

    const auto ColonPosition = reference.find(':', SlashPosition + 1U);
    if (ColonPosition == std::string::npos || ColonPosition == reference.size() - 1U)
    {
        throw std::runtime_error("plugin workflow reference '" + reference +
                                 "' must use the form 'organization/plugin:workflow'");
    }

    const std::string Organization = reference.substr(0, SlashPosition);
    const std::string Plugin =
        reference.substr(SlashPosition + 1U, ColonPosition - SlashPosition - 1U);
    const std::string WorkflowName = reference.substr(ColonPosition + 1U);

    if (Organization.empty() || Plugin.empty() || WorkflowName.empty())
    {
        throw std::runtime_error("plugin workflow reference '" + reference +
                                 "' must use the form 'organization/plugin:workflow'");
    }

    return PluginWorkflowRef {
        .organization = Organization,
        .plugin = Plugin,
        .workflowName = WorkflowName,
    };
}

}  // namespace beez::core
