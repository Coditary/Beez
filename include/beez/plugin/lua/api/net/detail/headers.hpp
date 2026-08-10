#pragma once

#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua::net_detail
{

using HeaderList = std::vector<std::pair<std::string, std::string>>;

[[nodiscard]] HeaderList parseHeadersTable(const sol::table& headersTable);

[[nodiscard]] HeaderList parseHeadersObject(const sol::object& headersValue);

}  // namespace beez::plugin::lua::net_detail
