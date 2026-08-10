#pragma once

#include "beez/plugin/lua/api/data/detail/format.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace beez::plugin::lua::data_detail
{

[[nodiscard]] std::optional<DataFormat> formatFromOptions(const sol::object& options);
[[nodiscard]] DataFormat resolveFormat(const std::filesystem::path& path,
                                       const sol::object& options);
[[nodiscard]] DataFormat resolveFormat(const sol::object& options);

[[nodiscard]] sol::table
deserializeString(sol::state& luaState, const std::string& content, DataFormat format);
[[nodiscard]] std::string serializeString(const sol::table& table, DataFormat format);

void serializeFile(const std::filesystem::path& path, const sol::table& table, DataFormat format);
[[nodiscard]] sol::table
deserializeFile(sol::state& luaState, const std::filesystem::path& path, DataFormat format);

}  // namespace beez::plugin::lua::data_detail
