#pragma once

#include <optional>
#include <string>

namespace beez::core
{

[[nodiscard]] std::optional<std::string> formatConfigOptions(const std::string& dottedPath);

}  // namespace beez::core
