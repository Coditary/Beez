#include "beez/plugin/lua/dsl/configure_parser.hpp"

#include <optional>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] bool isPresent(const sol::object& value)
{
    return value.valid() && value.get_type() != sol::type::lua_nil;
}

[[nodiscard]] std::string formatEntryIndex(const sol::object& key)
{
    if (key.is<int>())
    {
        return std::to_string(key.as<int>());
    }

    return "?";
}

[[nodiscard]] sol::table copyTableWithoutSteps(const std::shared_ptr<sol::state>& luaState,
                                               const sol::table& source)
{
    sol::table copy = luaState->create_table();
    source.for_each(
        [&copy](const sol::object& key, const sol::object& value)
        {
            if (key.is<std::string>() && key.as<std::string>() == "steps")
            {
                return;
            }

            copy[key] = value;
        });
    return copy;
}

void parsePluginSteps(
    const std::string& qualifiedPluginName,
    const sol::table& stepsTable,
    const std::optional<std::string>& profile,
    const std::function<void(const std::string& stepName,
                             const sol::table& stepConfig,
                             const std::optional<std::string>& profile)>& onStepConfig)
{
    stepsTable.for_each(
        [&qualifiedPluginName, &onStepConfig, &profile](const sol::object& key,
                                                        const sol::object& value)
        {
            if (!key.is<std::string>())
            {
                throw std::runtime_error("configure plugin '" + qualifiedPluginName +
                                         "' steps keys must be step name strings");
            }

            if (!value.is<sol::table>())
            {
                throw std::runtime_error("configure plugin '" + qualifiedPluginName + "' steps['" +
                                         key.as<std::string>() + "'] must be a table");
            }

            onStepConfig(key.as<std::string>(), value.as<sol::table>(), profile);
        });
}

[[nodiscard]] bool isStandaloneStepReference(const std::string& reference)
{
    return !reference.empty() && reference.front() == ':';
}

[[nodiscard]] std::string standaloneStepName(const std::string& reference)
{
    if (!isStandaloneStepReference(reference))
    {
        return reference;
    }

    if (reference.size() == 1U)
    {
        throw std::runtime_error("configure step reference ':' must include a step name");
    }

    return reference.substr(1);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
void parseConfigureEntry(
    const sol::object& key,
    const sol::object& value,
    const std::shared_ptr<sol::state>& luaState,
    const std::function<void(const std::string& qualifiedPluginName,
                             const sol::table& pluginConfig,
                             const std::optional<std::string>& profile)>& onPluginConfig,
    const std::function<void(const std::string& stepName,
                             const sol::table& stepConfig,
                             const std::optional<std::string>& profile)>& onStepConfig)
{
    const std::string EntryLabel = formatEntryIndex(key);

    if (!value.is<sol::table>())
    {
        throw std::runtime_error("configure entry at index " + EntryLabel + " must be a table");
    }

    const sol::table EntryTable = value.as<sol::table>();
    const sol::object TargetValue = EntryTable[1];
    const sol::object ConfigValue = EntryTable[2];

    if (!isPresent(TargetValue) || !TargetValue.is<std::string>() ||
        TargetValue.as<std::string>().empty())
    {
        throw std::runtime_error("configure entry at index " + EntryLabel +
                                 " must be { target, config_table }");
    }

    if (!isPresent(ConfigValue) || !ConfigValue.is<sol::table>())
    {
        throw std::runtime_error("configure entry at index " + EntryLabel +
                                 " config must be a table");
    }

    const std::string Target = TargetValue.as<std::string>();
    const sol::table ConfigTable = ConfigValue.as<sol::table>();

    std::optional<std::string> profile;
    const sol::object ProfileValue = EntryTable["profile"];
    if (isPresent(ProfileValue) && ProfileValue.is<std::string>())
    {
        profile = ProfileValue.as<std::string>();
    }

    if (isStandaloneStepReference(Target))
    {
        onStepConfig(standaloneStepName(Target), ConfigTable, profile);
        return;
    }

    onPluginConfig(Target, copyTableWithoutSteps(luaState, ConfigTable), profile);

    const sol::object StepsValue = ConfigTable["steps"];
    if (!isPresent(StepsValue))
    {
        return;
    }

    if (!StepsValue.is<sol::table>())
    {
        throw std::runtime_error("configure plugin '" + Target + "' field 'steps' must be a table");
    }

    parsePluginSteps(Target, StepsValue.as<sol::table>(), profile, onStepConfig);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

}  // namespace

void parseConfigureTable(
    const sol::table& entriesTable,
    const std::shared_ptr<sol::state>& luaState,
    const std::function<void(const std::string& qualifiedPluginName,
                             const sol::table& pluginConfig,
                             const std::optional<std::string>& profile)>& onPluginConfig,
    const std::function<void(const std::string& stepName,
                             const sol::table& stepConfig,
                             const std::optional<std::string>& profile)>& onStepConfig)
{
    if (entriesTable.empty())
    {
        return;
    }

    bool hasListEntry = false;
    entriesTable.for_each(
        [&](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                throw std::runtime_error(
                    "configure must be a list of { target, config_table } entries");
            }

            hasListEntry = true;
            parseConfigureEntry(key, value, luaState, onPluginConfig, onStepConfig);
        });

    if (!hasListEntry)
    {
        throw std::runtime_error("configure must contain at least one entry");
    }
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
