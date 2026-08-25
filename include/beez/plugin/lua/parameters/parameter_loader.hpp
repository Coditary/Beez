#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua::parameters
{

void applyParameterDefines(sol::table& varTable, const std::vector<std::string>& defines);

[[nodiscard]] std::vector<std::string> parseDefineArgument(const std::string& define);

void loadParameterFiles(sol::table& varTable,
                        const std::vector<std::string>& paths,
                        const std::filesystem::path& projectRoot,
                        const std::optional<std::string>& profile,
                        const std::vector<std::string>& defines);

}  // namespace beez::plugin::lua::parameters
