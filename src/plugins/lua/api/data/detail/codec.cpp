#include "beez/plugin/lua/api/data/detail/codec.hpp"

#include "beez/plugin/lua/api/data/detail/csv_convert.hpp"
#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"
#include "beez/plugin/lua/api/data/detail/toml_convert.hpp"
#include "beez/plugin/lua/api/data/detail/xml_convert.hpp"
#include "beez/plugin/lua/api/data/detail/yaml_convert.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <ryml.hpp>
#include <ryml_std.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,performance-unnecessary-value-param,readability-identifier-naming)
namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] std::string readFile(const std::filesystem::path& path)
{
    const std::ifstream Stream(path, std::ios::binary);
    if (!Stream)
    {
        throw std::runtime_error("beez.data: failed to read file '" + path.string() + "'");
    }

    std::ostringstream buffer;
    buffer << Stream.rdbuf();
    return buffer.str();
}

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        throw std::runtime_error("beez.data: failed to write file '" + path.string() + "'");
    }

    stream << content;
}

[[nodiscard]] ryml::ConstNodeRef effectiveYamlRoot(ryml::ConstNodeRef root)
{
    if (root.is_stream())
    {
        return root.first_child();
    }

    return root;
}

}  // namespace

std::optional<DataFormat> formatFromOptions(const sol::object& options)
{
    if (!options.valid() || options.is<sol::lua_nil_t>())
    {
        return std::nullopt;
    }

    if (options.is<std::string>())
    {
        return parseFormat(options.as<std::string>());
    }

    if (options.is<sol::table>())
    {
        const sol::table Table = options.as<sol::table>();
        const sol::object TypeValue = Table["type"];
        if (TypeValue.valid() && TypeValue.is<std::string>())
        {
            return parseFormat(TypeValue.as<std::string>());
        }
    }

    throw std::runtime_error("beez.data: format option must be a string or table with type field");
}

DataFormat resolveFormat(const std::filesystem::path& path, const sol::object& options)
{
    if (const std::optional<DataFormat> FromOptions = formatFromOptions(options))
    {
        return *FromOptions;
    }

    if (const std::optional<DataFormat> FromPath = formatFromPath(path))
    {
        return *FromPath;
    }

    throw std::runtime_error("beez.data: cannot determine format for '" + path.string() +
                             "' (use type option or a supported file extension)");
}

DataFormat resolveFormat(const sol::object& options)
{
    if (const std::optional<DataFormat> FromOptions = formatFromOptions(options))
    {
        return *FromOptions;
    }

    throw std::runtime_error("beez.data: format type is required for string operations");
}

sol::table
deserializeString(sol::state_view luaState, const std::string& content, const DataFormat Format)
{
    switch (Format)
    {
    case DataFormat::Json:
    {
        const YyjsonDocPtr Document = parseYyjsonDocument(content);
        return yyjsonValueToLua(luaState, yyjson_doc_get_root(Document.get())).as<sol::table>();
    }
    case DataFormat::Yaml:
    {
        std::string buffer = content;
        ryml::Tree tree;
        ryml::parse_in_place(ryml::to_substr(buffer), &tree);
        return rymlNodeToLua(luaState, effectiveYamlRoot(tree.rootref())).as<sol::table>();
    }
    case DataFormat::Xml:
        return xmlStringToLua(luaState, content);
    case DataFormat::Csv:
        return csvStringToLua(luaState, content);
    case DataFormat::Toml:
        return tomlStringToLua(luaState, content);
    }

    throw std::runtime_error("beez.data: unsupported format");
}

std::string serializeString(const sol::table& table, const DataFormat Format)
{
    switch (Format)
    {
    case DataFormat::Json:
    {
        const YyjsonMutDocPtr Document =
            luaToYyjsonDocument(sol::make_object(table.lua_state(), table));
        return yyjsonDocumentToString(*Document, true);
    }
    case DataFormat::Yaml:
        return luaTableToYamlString(table);
    case DataFormat::Xml:
        return luaTableToXmlString(table);
    case DataFormat::Csv:
        return luaTableToCsvString(table);
    case DataFormat::Toml:
        return luaTableToTomlString(table);
    }

    throw std::runtime_error("beez.data: unsupported format");
}

void serializeFile(const std::filesystem::path& path,
                   const sol::table& table,
                   const DataFormat Format)
{
    writeFile(path, serializeString(table, Format));
}

sol::table deserializeFile(sol::state_view luaState,
                           const std::filesystem::path& path,
                           const DataFormat Format)
{
    return deserializeString(luaState, readFile(path), Format);
}

}  // namespace beez::plugin::lua::data_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,performance-unnecessary-value-param,readability-identifier-naming)
