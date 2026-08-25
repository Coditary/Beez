#include "beez/plugin/lua/parameters/parameter_loader.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

#include <yyjson.h>

namespace beez::plugin::lua::parameters
{

namespace
{

[[nodiscard]] bool isMergeableTable(const sol::object& object)
{
    return object.get_type() == sol::type::table &&
           !data_detail::isLuaArray(object.as<sol::table>());
}

// NOLINTNEXTLINE(misc-no-recursion)
void deepMergeInto(const sol::state_view& luaState, sol::table& base, const sol::table& overlay)
{
    overlay.for_each(
        // NOLINTNEXTLINE(misc-no-recursion)
        [&luaState, &base](const sol::object& key, const sol::object& value)
        {
            const sol::object Existing = base[key];

            if (isMergeableTable(Existing) && isMergeableTable(value))
            {
                sol::table existingTable = Existing;
                const sol::table ValueTable = value;
                deepMergeInto(luaState, existingTable, ValueTable);
                return;
            }

            // Values set via -D from the CLI always win: a scalar that already
            // exists in the store is never replaced by a JSON table or array.
            if (Existing.get_type() != sol::type::lua_nil &&
                Existing.get_type() != sol::type::table && value.get_type() == sol::type::table)
            {
                return;
            }

            base[key] = value;
        });
}

void validateParameterDocument(const sol::table& document, const std::string& sourceName)
{
    for (const auto& [key, value] : document)
    {
        if (!key.is<std::string>())
        {
            throw std::runtime_error("parameters: keys in '" + sourceName + "' must be strings");
        }

        const auto KeyString = key.as<std::string>();
        if (KeyString != "properties" && KeyString != "profiles")
        {
            std::cerr << "Warning: unknown parameters key '" << KeyString << "' in '" << sourceName
                      << "'\n";
            continue;
        }

        if (value.get_type() != sol::type::table)
        {
            std::string message = "parameters: '" + KeyString;
            message += "' in '" + sourceName;
            message += "' must be a table";
            throw std::runtime_error(message);
        }
    }

    const sol::object Profiles = document["profiles"];
    if (Profiles.get_type() == sol::type::table)
    {
        Profiles.as<sol::table>().for_each(
            [&sourceName](const sol::object& name, const sol::object& profile)
            {
                if (!name.is<std::string>())
                {
                    throw std::runtime_error("parameters: profile names in '" + sourceName +
                                             "' must be strings");
                }
                if (profile.get_type() != sol::type::table)
                {
                    std::string message = "parameters: profile '" + name.as<std::string>();
                    message += "' in '" + sourceName;
                    message += "' must be a table";
                    throw std::runtime_error(message);
                }
            });
    }
}

[[nodiscard]] sol::object parseParameterFile(const sol::state_view& luaState,
                                             const std::filesystem::path& resolvedPath,
                                             const std::string& displayPath)
{
    std::ifstream input(resolvedPath);
    if (!input.is_open())
    {
        throw std::runtime_error("parameters: cannot open parameter file '" + displayPath + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    data_detail::YyjsonDocPtr document;
    try
    {
        document = data_detail::parseYyjsonDocument(buffer.str());
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("parameters: failed to parse JSON in '" + displayPath + "'");
    }

    yyjson_val* root = yyjson_doc_get_root(document.get());
    if (yyjson_get_type(root) != YYJSON_TYPE_OBJ)
    {
        throw std::runtime_error("parameters: '" + displayPath + "' must contain a JSON object");
    }

    return data_detail::yyjsonValueToLua(luaState, root);
}

}  // namespace

std::vector<std::string> parseDefineArgument(const std::string& define)
{
    const auto Separator = define.find('=');
    if (Separator == std::string::npos || Separator == 0)
    {
        throw std::runtime_error("parameter define '" + define + "' must use the form KEY=VALUE");
    }

    return {define.substr(0, Separator), define.substr(Separator + 1)};
}

void applyParameterDefines(sol::table& varTable, const std::vector<std::string>& defines)
{
    if (defines.empty())
    {
        return;
    }

    sol::state_view luaState(varTable.lua_state());

    for (const auto& define : defines)
    {
        const auto Parts = parseDefineArgument(define);
        const auto& defineKey = Parts.at(0);
        const auto& defineValue = Parts.at(1);

        std::vector<std::string> parts;
        std::size_t start = 0;
        while (true)
        {
            const auto Dot = defineKey.find('.', start);
            if (Dot == std::string::npos)
            {
                parts.push_back(defineKey.substr(start));
                break;
            }
            parts.push_back(defineKey.substr(start, Dot - start));
            start = Dot + 1;
        }

        sol::table current = varTable;
        for (std::size_t index = 0; index + 1U < parts.size(); ++index)
        {
            const auto& part = parts.at(index);
            const sol::object Next = current[part];
            if (Next.get_type() == sol::type::lua_nil)
            {
                const sol::table Nested = luaState.create_table();
                current[part] = Nested;
                current = Nested;
            }
            else if (Next.get_type() == sol::type::table)
            {
                current = Next.as<sol::table>();
            }
            else
            {
                std::string message = "parameter define '" + defineKey;
                message += "' conflicts with an existing non-table value at '";
                message += part;
                message += "'";
                throw std::runtime_error(message);
            }
        }

        current[parts.back()] = defineValue;
    }
}

void loadParameterFiles(sol::table& varTable,
                        const std::vector<std::string>& paths,
                        const std::filesystem::path& projectRoot,
                        const std::optional<std::string>& profile,
                        const std::vector<std::string>& defines)
{
    const sol::state_view LuaState(varTable.lua_state());

    for (const auto& rawPath : paths)
    {
        if (rawPath.empty())
        {
            throw std::runtime_error("parameters: file paths must be non-empty strings");
        }

        const std::filesystem::path Resolved = projectRoot / std::filesystem::path(rawPath);

        const sol::object Document = parseParameterFile(LuaState, Resolved, rawPath);
        if (Document.get_type() != sol::type::table)
        {
            throw std::runtime_error("parameters: '" + rawPath + "' must contain a JSON object");
        }

        const sol::table DocumentTable = Document.as<sol::table>();
        validateParameterDocument(DocumentTable, rawPath);

        const sol::object Properties = DocumentTable["properties"];
        if (Properties.get_type() == sol::type::table)
        {
            deepMergeInto(LuaState, varTable, Properties.as<sol::table>());
        }

        const sol::object Profiles = DocumentTable["profiles"];
        if (profile.has_value() && Profiles.get_type() == sol::type::table)
        {
            const sol::object Selected = Profiles.as<sol::table>()[*profile];
            if (Selected.get_type() == sol::type::table)
            {
                deepMergeInto(LuaState, varTable, Selected.as<sol::table>());
            }
        }
    }

    applyParameterDefines(varTable, defines);
}

}  // namespace beez::plugin::lua::parameters
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)