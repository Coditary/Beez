#include "beez/plugin/lua/api/text/detail/prebyte_backend.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,performance-enum-size,readability-identifier-naming,bugprone-easily-swappable-parameters,misc-no-recursion,modernize-return-braced-init-list,misc-const-correctness)
#include <Engine.h>
#include <sol/sol.hpp>
#include <support/Diagnostic.h>

namespace beez::plugin::lua::text_detail
{

namespace
{

[[nodiscard]] bool isWholeNumber(const double value)
{
    return std::floor(value) == value;
}

[[nodiscard]] prebyte::Data luaObjectToData(const sol::object& object)
{
    if (!object.valid() || object.is<sol::lua_nil_t>())
    {
        return prebyte::Data {};
    }

    if (object.is<bool>())
    {
        return prebyte::Data(object.as<bool>());
    }

    if (object.is<std::string>())
    {
        return prebyte::Data(object.as<std::string>());
    }

    if (object.is<int>())
    {
        return prebyte::Data(object.as<int>());
    }

    if (object.is<double>())
    {
        const double Value = object.as<double>();
        if (isWholeNumber(Value))
        {
            return prebyte::Data(static_cast<int>(Value));
        }
        return prebyte::Data(Value);
    }

    if (object.is<sol::table>())
    {
        const sol::table Table = object.as<sol::table>();
        if (data_detail::isLuaArray(Table))
        {
            // Prebyte list indexing is 0-based; Lua tables are 1-based. Store sequential
            // Lua arrays as maps keyed by "1", "2", ... so {{ items[1] }} matches Lua.
            prebyte::Data::Map map;
            for (std::size_t index = 1; index <= Table.size(); ++index)
            {
                map[std::to_string(index)] = luaObjectToData(Table[index]);
            }
            return prebyte::Data(std::move(map));
        }

        prebyte::Data::Map map;
        Table.for_each([&map](const sol::object& key, const sol::object& value)
                       { map[key.as<std::string>()] = luaObjectToData(value); });
        return prebyte::Data(std::move(map));
    }

    throw std::runtime_error("beez.text.template: unsupported Lua value type in variables table");
}

[[nodiscard]] prebyte::Value luaObjectToValue(const sol::object& object)
{
    return prebyte::Value::from_data(luaObjectToData(object));
}

[[nodiscard]] prebyte::RenderContext buildRenderContext(const sol::table& variables)
{
    prebyte::RenderContext context;
    variables.for_each([&context](const sol::object& key, const sol::object& value)
                       { context.set(key.as<std::string>(), luaObjectToValue(value)); });
    return context;
}

[[nodiscard]] prebyte::Engine& lazyEngine()
{
    static std::once_flag initFlag;
    static std::unique_ptr<prebyte::Engine> engine;
    std::call_once(initFlag, []() { engine = std::make_unique<prebyte::Engine>(); });
    return *engine;
}

}  // namespace

std::string renderTemplateString(const std::string& templateString, const sol::table& variables)
{
    try
    {
        prebyte::Engine& engine = lazyEngine();
        const prebyte::CompiledTemplate compiled = engine.compile(templateString);
        const prebyte::RenderContext context = buildRenderContext(variables);
        return engine.render(compiled, context);
    }
    catch (const prebyte::DiagnosticError& error)
    {
        throw std::runtime_error(std::string("beez.text.template: ") + error.what());
    }
}

}  // namespace beez::plugin::lua::text_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,performance-enum-size,readability-identifier-naming,bugprone-easily-swappable-parameters,misc-no-recursion,modernize-return-braced-init-list,misc-const-correctness)
