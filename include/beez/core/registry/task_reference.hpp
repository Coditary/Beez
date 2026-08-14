#pragma once

#include <string>

namespace beez::core
{

struct PluginTaskRef
{
    std::string organization;
    std::string plugin;
    std::string taskName;
};

[[nodiscard]] std::string formatPluginTaskKey(const std::string& organization,
                                            const std::string& plugin,
                                            const std::string& taskName);

[[nodiscard]] PluginTaskRef parsePluginTaskReference(const std::string& reference);

[[nodiscard]] bool looksLikePluginTaskReference(const std::string& reference);

}  // namespace beez::core
