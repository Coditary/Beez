#include "beez/core/registry/step_reference.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

namespace beez::core
{

namespace
{

[[nodiscard]] std::pair<std::string, std::string> splitPluginName(const std::string& qualifiedName)
{
    const auto SlashPosition = qualifiedName.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0 ||
        SlashPosition == qualifiedName.size() - 1)
    {
        throw std::runtime_error("plugin name '" + qualifiedName +
                                 "' must use the form 'organization/plugin'");
    }

    return {qualifiedName.substr(0, SlashPosition), qualifiedName.substr(SlashPosition + 1)};
}

[[nodiscard]] std::string stripPluginVersionSuffix(const std::string& pluginPart)
{
    const auto AtPosition = pluginPart.rfind('@');
    if (AtPosition == std::string::npos || AtPosition == 0 || AtPosition == pluginPart.size() - 1)
    {
        return pluginPart;
    }

    return pluginPart.substr(0, AtPosition);
}

}  // namespace

std::string formatQualifiedStepRef(const std::string& organization,
                                   const std::string& plugin,
                                   const std::string& stepName)
{
    return organization + '/' + plugin + ':' + stepName;
}

std::string formatVersionedQualifiedStepRef(const std::string& organization,
                                            const std::string& plugin,
                                            const std::string& version,
                                            const std::string& stepName)
{
    return organization + '/' + plugin + '@' + version + ':' + stepName;
}

std::string formatShortPluginStepRef(const std::string& plugin, const std::string& stepName)
{
    return plugin + ':' + stepName;
}

std::string formatVersionedInvocationRef(const std::string& baseReference,
                                         const std::string& version)
{
    return baseReference + '@' + version;
}

std::string stepActionName(const std::string& stepName)
{
    const auto ColonPosition = stepName.find(':');
    if (ColonPosition == std::string::npos)
    {
        return stepName;
    }

    return stepName.substr(0, ColonPosition);
}

bool isDefaultScopedStepName(const std::string& stepName)
{
    constexpr std::string_view DefaultScope = "code";
    const auto ColonPosition = stepName.find(':');
    if (ColonPosition == std::string::npos)
    {
        return false;
    }

    return stepName.substr(ColonPosition + 1) == DefaultScope;
}

std::pair<std::string, std::optional<std::string>>
splitStepReferenceVersion(const std::string& reference)
{
    const auto AtPosition = reference.rfind('@');
    if (AtPosition == std::string::npos || AtPosition == 0 || AtPosition == reference.size() - 1)
    {
        return {reference, std::nullopt};
    }

    return {reference.substr(0, AtPosition), reference.substr(AtPosition + 1)};
}

std::optional<QualifiedStepRef> parseQualifiedStepRef(const std::string& reference)
{
    const auto [BaseReference, Version] = splitStepReferenceVersion(reference);
    (void)Version;
    const auto SlashPosition = BaseReference.find('/');
    if (SlashPosition == std::string::npos)
    {
        return std::nullopt;
    }

    const auto ColonPosition = BaseReference.find(':', SlashPosition);
    if (ColonPosition == std::string::npos || ColonPosition == BaseReference.size() - 1)
    {
        return std::nullopt;
    }

    const std::string PluginPart = stripPluginVersionSuffix(BaseReference.substr(0, ColonPosition));
    const auto [Organization, Plugin] = splitPluginName(PluginPart);
    return QualifiedStepRef {.organization = Organization,
                             .plugin = Plugin,
                             .stepName = BaseReference.substr(ColonPosition + 1),
                             .version = Version};
}

std::optional<std::pair<std::string, std::string>>
parseShortPluginStepRef(const std::string& reference)
{
    const auto [BaseReference, Version] = splitStepReferenceVersion(reference);
    (void)Version;
    if (BaseReference.find('/') != std::string::npos)
    {
        return std::nullopt;
    }

    const auto ColonPosition = BaseReference.find(':');
    if (ColonPosition == std::string::npos || ColonPosition == 0 ||
        ColonPosition == BaseReference.size() - 1)
    {
        return std::nullopt;
    }

    const std::string Plugin = stripPluginVersionSuffix(BaseReference.substr(0, ColonPosition));
    return std::pair {Plugin, BaseReference.substr(ColonPosition + 1)};
}

std::optional<PluginIdentity> extractPluginIdentity(const std::string& reference)
{
    const auto [BaseReference, Version] = splitStepReferenceVersion(reference);
    (void)Version;
    (void)Version;

    if (const auto Qualified = parseQualifiedStepRef(BaseReference))
    {
        return PluginIdentity {.organization = Qualified->organization,
                               .plugin = Qualified->plugin};
    }

    if (const auto ShortPlugin = parseShortPluginStepRef(BaseReference))
    {
        return PluginIdentity {.organization = std::nullopt, .plugin = ShortPlugin->first};
    }

    return std::nullopt;
}

std::string formatPluginVersionKey(const std::string& organization,
                                   const std::string& plugin,
                                   const std::string& version)
{
    return organization + '/' + plugin + '@' + version;
}

}  // namespace beez::core
