#include "beez/plugin/lua/api/data/detail/xml_convert.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] sol::object xmlNodeToLua(sol::state_view luaState, rapidxml::xml_node<>* node)
{
    if (node == nullptr)
    {
        return sol::lua_nil;
    }

    if (node->type() != rapidxml::node_element)
    {
        if (node->value() != nullptr)
        {
            return sol::make_object(luaState, std::string(node->value()));
        }
        return sol::lua_nil;
    }

    sol::table result = luaState.create_table();
    result["tag"] = std::string(node->name());

    sol::table attributes = luaState.create_table();
    bool hasAttributes = false;
    for (rapidxml::xml_attribute<>* attribute = node->first_attribute(); attribute != nullptr;
         attribute = attribute->next_attribute())
    {
        attributes[attribute->name()] = attribute->value();
        hasAttributes = true;
    }
    if (hasAttributes)
    {
        result["attrs"] = attributes;
    }

    std::string text;
    sol::table children = luaState.create_table();
    std::size_t childIndex = 1;
    for (rapidxml::xml_node<>* child = node->first_node(); child != nullptr;
         child = child->next_sibling())
    {
        if (child->type() == rapidxml::node_data || child->type() == rapidxml::node_cdata)
        {
            if (child->value() != nullptr)
            {
                text += child->value();
            }
            continue;
        }

        if (child->type() == rapidxml::node_element)
        {
            children[childIndex] = xmlNodeToLua(luaState, child);
            ++childIndex;
        }
    }

    if (!text.empty())
    {
        result["text"] = text;
    }

    if (childIndex > 1)
    {
        result["children"] = children;
    }

    return result;
}

void buildXmlNode(rapidxml::xml_document<>& document,
                  rapidxml::xml_node<>* parent,
                  const sol::table& table)
{
    const sol::object TagValue = table["tag"];
    if (!TagValue.valid() || !TagValue.is<std::string>())
    {
        throw std::runtime_error("beez.data: XML table must contain a string 'tag' field");
    }

    const std::string Tag = TagValue.as<std::string>();
    rapidxml::xml_node<>* node =
        document.allocate_node(rapidxml::node_element,
                               document.allocate_string(Tag.c_str()),
                               nullptr);
    parent->append_node(node);

    const sol::object Attributes = table["attrs"];
    if (Attributes.valid() && Attributes.is<sol::table>())
    {
        Attributes.as<sol::table>().for_each(
            [&document, &node](const sol::object& key, const sol::object& value)
            {
                if (!value.is<std::string>())
                {
                    throw std::runtime_error("beez.data: XML attribute values must be strings");
                }

                const std::string Key = key.as<std::string>();
                const std::string Value = value.as<std::string>();
                node->append_attribute(document.allocate_attribute(
                    document.allocate_string(Key.c_str()),
                    document.allocate_string(Value.c_str())));
            });
    }

    const sol::object TextValue = table["text"];
    if (TextValue.valid() && TextValue.is<std::string>())
    {
        const std::string Text = TextValue.as<std::string>();
        node->append_node(document.allocate_node(rapidxml::node_data,
                                                 nullptr,
                                                 document.allocate_string(Text.c_str())));
    }

    const sol::object Children = table["children"];
    if (Children.valid() && Children.is<sol::table>())
    {
        Children.as<sol::table>().for_each(
            [&document, &node](const sol::object& /*key*/, const sol::object& value)
            {
                if (!value.is<sol::table>())
                {
                    throw std::runtime_error("beez.data: XML children must be tables");
                }

                buildXmlNode(document, node, value.as<sol::table>());
            });
    }
}

}  // namespace

sol::table xmlStringToLua(sol::state_view luaState, const std::string& content)
{
    std::vector<char> buffer(content.begin(), content.end());
    buffer.push_back('\0');

    rapidxml::xml_document<> document;
    try
    {
        document.parse<rapidxml::parse_default>(&buffer[0]);
    }
    catch (const rapidxml::parse_error& error)
    {
        throw std::runtime_error(std::string("beez.data: failed to parse XML: ") + error.what());
    }

    rapidxml::xml_node<>* root = nullptr;
    for (rapidxml::xml_node<>* node = document.first_node(); node != nullptr;
         node = node->next_sibling())
    {
        if (node->type() == rapidxml::node_element)
        {
            root = node;
            break;
        }
    }

    if (root == nullptr)
    {
        throw std::runtime_error("beez.data: XML document has no root node");
    }

    return xmlNodeToLua(luaState, root).as<sol::table>();
}

std::string luaTableToXmlString(const sol::table& table)
{
    rapidxml::xml_document<> document;
    buildXmlNode(document, &document, table);

    std::string output;
    rapidxml::print(std::back_inserter(output), document, rapidxml::print_no_indenting);
    return output;
}

}  // namespace beez::plugin::lua::data_detail
