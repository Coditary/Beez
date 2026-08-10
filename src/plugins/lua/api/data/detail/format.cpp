#include "beez/plugin/lua/api/data/detail/format.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

}  // namespace

DataFormat parseFormat(const std::string_view type)
{
    const std::string Normalized = toLower(std::string(type));
    if (Normalized == "json")
    {
        return DataFormat::Json;
    }

    if (Normalized == "yaml" || Normalized == "yml")
    {
        return DataFormat::Yaml;
    }

    if (Normalized == "xml")
    {
        return DataFormat::Xml;
    }

    if (Normalized == "csv")
    {
        return DataFormat::Csv;
    }

    if (Normalized == "toml")
    {
        return DataFormat::Toml;
    }

    throw std::runtime_error("beez.data: unsupported format '" + std::string(type) +
                             "' (supported: json, yaml, xml, csv, toml)");
}

std::optional<DataFormat> formatFromPath(const std::filesystem::path& path)
{
    const std::string Extension = toLower(path.extension().string());
    if (Extension == ".json")
    {
        return DataFormat::Json;
    }

    if (Extension == ".yaml" || Extension == ".yml")
    {
        return DataFormat::Yaml;
    }

    if (Extension == ".xml")
    {
        return DataFormat::Xml;
    }

    if (Extension == ".csv")
    {
        return DataFormat::Csv;
    }

    if (Extension == ".toml")
    {
        return DataFormat::Toml;
    }

    return std::nullopt;
}

std::string formatToExtension(const DataFormat format)
{
    switch (format)
    {
    case DataFormat::Json:
        return ".json";
    case DataFormat::Yaml:
        return ".yaml";
    case DataFormat::Xml:
        return ".xml";
    case DataFormat::Csv:
        return ".csv";
    case DataFormat::Toml:
        return ".toml";
    }

    return ".json";
}

}  // namespace beez::plugin::lua::data_detail
