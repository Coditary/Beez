#include "beez/core/registry/task_reference.hpp"

#include <stdexcept>
#include <string>

namespace beez::core
{

std::string formatPluginTaskKey(const std::string& organization,
                                const std::string& plugin,
                                const std::string& taskName)
{
    return organization + '/' + plugin + ':' + taskName;
}

PluginTaskRef parsePluginTaskReference(const std::string& reference)
{
    const auto SlashPosition = reference.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0U)
    {
        throw std::runtime_error("plugin task reference '" + reference +
                                 "' must use the form 'organization/plugin:task'");
    }

    const auto ColonPosition = reference.find(':', SlashPosition + 1U);
    if (ColonPosition == std::string::npos || ColonPosition == reference.size() - 1U)
    {
        throw std::runtime_error("plugin task reference '" + reference +
                                 "' must use the form 'organization/plugin:task'");
    }

    const std::string Organization = reference.substr(0, SlashPosition);
    const std::string Plugin =
        reference.substr(SlashPosition + 1U, ColonPosition - SlashPosition - 1U);
    const std::string TaskName = reference.substr(ColonPosition + 1U);

    if (Organization.empty() || Plugin.empty() || TaskName.empty())
    {
        throw std::runtime_error("plugin task reference '" + reference +
                                 "' must use the form 'organization/plugin:task'");
    }

    return PluginTaskRef {
        .organization = Organization,
        .plugin = Plugin,
        .taskName = TaskName,
    };
}

bool looksLikePluginTaskReference(const std::string& reference)
{
    if (reference.empty())
    {
        return false;
    }

    const auto SlashPosition = reference.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0U)
    {
        return false;
    }

    const auto ColonPosition = reference.find(':', SlashPosition + 1U);
    return ColonPosition != std::string::npos && ColonPosition < reference.size() - 1U;
}

}  // namespace beez::core
