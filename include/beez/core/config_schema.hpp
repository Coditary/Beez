#pragma once

#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

[[nodiscard]] std::optional<std::string> formatConfigOptions(const std::string& dottedPath);

// Returns dotted config paths for shell tab completion.
[[nodiscard]] std::vector<std::string> listConfigOptionCompletions(const std::string& prefix);

}  // namespace beez::core
