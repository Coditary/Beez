#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua::crypto_detail
{

inline sol::table algorithmsToTable(const std::shared_ptr<sol::state>& luaState,
                                    const std::vector<std::string>& algorithms)
{
    sol::table result = luaState->create_table();
    for (std::size_t index = 0; index < algorithms.size(); ++index)
    {
        result.set(static_cast<int>(index + 1), algorithms.at(index));
    }

    return result;
}

}  // namespace beez::plugin::lua::crypto_detail
