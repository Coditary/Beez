#pragma once

#include <optional>
#include <string>
#include <utility>

namespace beez::core
{

struct QualifiedStepRef
{
    std::string organization;
    std::string plugin;
    std::string stepName;
    std::optional<std::string> version;
};

struct PluginIdentity
{
    std::optional<std::string> organization;
    std::string plugin;
};

[[nodiscard]] std::string formatQualifiedStepRef(const std::string& organization,
                                                 const std::string& plugin,
                                                 const std::string& stepName);

[[nodiscard]] std::string formatVersionedQualifiedStepRef(const std::string& organization,
                                                          const std::string& plugin,
                                                          const std::string& version,
                                                          const std::string& stepName);

[[nodiscard]] std::string formatShortPluginStepRef(const std::string& plugin,
                                                   const std::string& stepName);

[[nodiscard]] std::string formatVersionedInvocationRef(const std::string& baseReference,
                                                       const std::string& version);

[[nodiscard]] std::string stepActionName(const std::string& stepName);

[[nodiscard]] bool isDefaultScopedStepName(const std::string& stepName);

[[nodiscard]] std::pair<std::string, std::optional<std::string>>
splitStepReferenceVersion(const std::string& reference);

[[nodiscard]] std::optional<QualifiedStepRef> parseQualifiedStepRef(const std::string& reference);

[[nodiscard]] std::optional<std::pair<std::string, std::string>>
parseShortPluginStepRef(const std::string& reference);

[[nodiscard]] std::optional<PluginIdentity> extractPluginIdentity(const std::string& reference);

[[nodiscard]] std::string formatPluginVersionKey(const std::string& organization,
                                                 const std::string& plugin,
                                                 const std::string& version);

}  // namespace beez::core
