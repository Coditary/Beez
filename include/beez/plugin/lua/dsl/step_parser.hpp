#pragma once

#include "beez/core/step.hpp"

#include <memory>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] std::vector<std::string> parseStringArrayField(const sol::table& options,
                                                             const std::string& fieldName,
                                                             const std::string& stepName);

[[nodiscard]] core::Step parseStepTable(const sol::table& options,
                                        const std::shared_ptr<sol::state>& luaState);

}  // namespace beez::plugin::lua
