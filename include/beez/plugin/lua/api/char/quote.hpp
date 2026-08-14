#pragma once

#include <string>

namespace beez::plugin::lua
{

[[nodiscard]] std::string shellQuote(const std::string& value);

}  // namespace beez::plugin::lua
