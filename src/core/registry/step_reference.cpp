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

}  // namespace

std::string formatQualifiedStepRef(const std::string& organization,
                                   const std::string& plugin,
                                   const std::string& stepName)
{
    return organization + '/' + plugin + ':' + stepName;
}

std::string formatShortPluginStepRef(const std::string& plugin, const std::string& stepName)
{
    return plugin + ':' + stepName;
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

std::optional<QualifiedStepRef> parseQualifiedStepRef(const std::string& reference)
{
    const auto SlashPosition = reference.find('/');
    if (SlashPosition == std::string::npos)
    {
        return std::nullopt;
    }

    const auto ColonPosition = reference.find(':', SlashPosition);
    if (ColonPosition == std::string::npos || ColonPosition == reference.size() - 1)
    {
        return std::nullopt;
    }

    const auto [Organization, Plugin] = splitPluginName(reference.substr(0, ColonPosition));
    return QualifiedStepRef {.organization = Organization,
                             .plugin = Plugin,
                             .stepName = reference.substr(ColonPosition + 1)};
}

std::optional<std::pair<std::string, std::string>>
parseShortPluginStepRef(const std::string& reference)
{
    if (reference.find('/') != std::string::npos)
    {
        return std::nullopt;
    }

    const auto ColonPosition = reference.find(':');
    if (ColonPosition == std::string::npos || ColonPosition == 0 ||
        ColonPosition == reference.size() - 1)
    {
        return std::nullopt;
    }

    return std::pair {reference.substr(0, ColonPosition), reference.substr(ColonPosition + 1)};
}

}  // namespace beez::core
