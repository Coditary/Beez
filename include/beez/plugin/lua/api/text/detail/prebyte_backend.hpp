#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

#include <string>

namespace beez::plugin::lua::text_detail
{

[[nodiscard]] std::string renderTemplateString(const std::string& templateString, const sol::table& variables);

}  // namespace beez::plugin::lua::text_detail
