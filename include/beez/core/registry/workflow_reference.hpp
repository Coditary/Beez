#pragma once

#include <string>
#include <tuple>

namespace beez::core
{

struct PluginWorkflowRef
{
    std::string organization;
    std::string plugin;
    std::string workflowName;
};

[[nodiscard]] std::string formatPluginWorkflowKey(const std::string& organization,
                                                  const std::string& plugin,
                                                  const std::string& workflowName);

[[nodiscard]] PluginWorkflowRef parsePluginWorkflowReference(const std::string& reference);

}  // namespace beez::core
