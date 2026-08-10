#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace beez::plugin::lua::data_detail
{

enum class DataFormat
{
    Json,
    Yaml,
    Xml,
    Csv,
    Toml,
};

[[nodiscard]] DataFormat parseFormat(std::string_view type);
[[nodiscard]] std::optional<DataFormat> formatFromPath(const std::filesystem::path& path);
[[nodiscard]] std::string formatToExtension(DataFormat format);

}  // namespace beez::plugin::lua::data_detail
