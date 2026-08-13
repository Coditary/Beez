#pragma once

#include <optional>
#include <string>

namespace beez::core
{

struct QualifiedStepRef
{
    std::string organization;
    std::string plugin;
    std::string stepName;
};

[[nodiscard]] std::string
formatQualifiedStepRef(const std::string& organization,
                       const std::string& plugin,
                       const std::string& stepName);

[[nodiscard]] std::string formatShortPluginStepRef(const std::string& plugin,
                                                   const std::string& stepName);

[[nodiscard]] std::string stepActionName(const std::string& stepName);

[[nodiscard]] bool isDefaultScopedStepName(const std::string& stepName);

[[nodiscard]] std::optional<QualifiedStepRef> parseQualifiedStepRef(const std::string& reference);

[[nodiscard]] std::optional<std::pair<std::string, std::string>>
parseShortPluginStepRef(const std::string& reference);

}  // namespace beez::core
