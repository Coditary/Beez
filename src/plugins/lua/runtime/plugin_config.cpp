#include "beez/plugin/lua/runtime/plugin_config.hpp"

#include "beez/plugin/lua/runtime/lua_table_util.hpp"
#include "beez/plugin/lua/runtime/step_config.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

struct PluginConfigDefinition
{
    std::shared_ptr<sol::state> luaState;
    sol::table defaults;
    std::unordered_map<std::string, sol::table> profileDefs;
    LuaStepConfigOptions options;
};

std::unordered_map<std::string, PluginConfigDefinition>& pluginConfigDefinitions()
{
    static std::unordered_map<std::string, PluginConfigDefinition> definitions;
    return definitions;
}

[[nodiscard]] sol::table
requireTableField(const sol::table& table, const char* fieldName, const std::string& pluginKey)
{
    const sol::object Value = table[fieldName];
    if (!Value.valid() || !Value.is<sol::table>())
    {
        throw std::runtime_error("plugin '" + pluginKey + "' config requires table field '" +
                                 fieldName + "'");
    }

    return Value.as<sol::table>();
}

[[nodiscard]] sol::table resolveStepConfigTable(const PluginConfigDefinition& definition,
                                                const sol::table& stepConfigTable)
{
    sol::table resolved = cloneLuaTable(definition.luaState, definition.defaults);

    const sol::object ProfileValue = stepConfigTable["profile"];
    if (ProfileValue.valid() && ProfileValue.is<std::string>() && !definition.profileDefs.empty())
    {
        const std::string ProfileName = ProfileValue.as<std::string>();
        const auto ProfileIterator = definition.profileDefs.find(ProfileName);
        if (ProfileIterator == definition.profileDefs.end())
        {
            throw std::runtime_error("plugin step references unknown config profile '" +
                                     ProfileName + "'");
        }

        deepMergeLuaTables(resolved, ProfileIterator->second);
    }

    sol::table stepOverlay = cloneLuaTable(definition.luaState, stepConfigTable);
    deepMergeLuaTables(resolved, stepOverlay);
    return resolved;
}

}  // namespace

void clearPluginConfigRegistry()
{
    pluginConfigDefinitions().clear();
}

void registerPluginConfigDefinition(const std::string& pluginKey,
                                    const std::shared_ptr<sol::state>& luaState,
                                    const sol::table& configTable)
{
    PluginConfigDefinition definition;
    definition.luaState = luaState;
    definition.defaults =
        cloneLuaTable(luaState, requireTableField(configTable, "defaults", pluginKey));

    const sol::object ProfileDefsValue = configTable["profile_defs"];
    if (ProfileDefsValue.valid() && ProfileDefsValue.is<sol::table>())
    {
        ProfileDefsValue.as<sol::table>().for_each(
            [&definition, &pluginKey](const sol::object& key, const sol::object& value)
            {
                if (!key.is<std::string>())
                {
                    throw std::runtime_error("plugin '" + pluginKey +
                                             "' config.profile_defs keys must be strings");
                }

                if (!value.is<sol::table>())
                {
                    throw std::runtime_error("plugin '" + pluginKey +
                                             "' config.profile_defs values must be tables");
                }

                definition.profileDefs.emplace(
                    key.as<std::string>(),
                    cloneLuaTable(definition.luaState, value.as<sol::table>()));
            });
    }

    const sol::object SchemaValue = configTable["schema"];
    if (SchemaValue.valid() && SchemaValue.is<sol::table>())
    {
        definition.options.schema = cloneLuaTable(luaState, SchemaValue.as<sol::table>());
    }

    const sol::object FinalizeValue = configTable["finalize"];
    if (FinalizeValue.valid() && !FinalizeValue.is<sol::lua_nil_t>())
    {
        if (!FinalizeValue.is<sol::protected_function>())
        {
            throw std::runtime_error("plugin '" + pluginKey +
                                     "' config.finalize must be a function");
        }

        definition.options.finalize = FinalizeValue.as<sol::protected_function>();
    }

    pluginConfigDefinitions()[pluginKey] = std::move(definition);
}

core::StepConfigPtr makePluginStepConfig(const std::shared_ptr<sol::state>& luaState,
                                         const std::string& pluginKey,
                                         const sol::table& stepConfigTable)
{
    const auto DefinitionIterator = pluginConfigDefinitions().find(pluginKey);
    if (DefinitionIterator == pluginConfigDefinitions().end())
    {
        return makeLuaStepConfig(luaState, stepConfigTable);
    }

    const PluginConfigDefinition& definition = DefinitionIterator->second;
    return makeLuaStepConfig(
        luaState,
        [definition, stepConfigTable]() -> sol::table
        { return resolveStepConfigTable(definition, stepConfigTable); },
        definition.options);
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
