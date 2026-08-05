#pragma once

#include "beez/core/registry.h"

#include <string>
#include <vector>

namespace beez::cli
{

[[nodiscard]] std::vector<std::string> collectEntityNames(const core::Registry& registry,
                                                          const std::string& kind);

[[nodiscard]] std::string formatEntityList(const std::string& kind,
                                           const std::vector<std::string>& names);

}  // namespace beez::cli
