#pragma once

#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/pattern.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/sol.hpp>

namespace beez::plugin::lua::fs_detail
{

inline sol::table globPatternsToTable(const std::shared_ptr<sol::state>& luaState,
                                      const std::vector<std::string>& patterns,
                                      const std::filesystem::path& projectRoot,
                                      core::GlobMetadataCache* metadataCache = nullptr)
{
    const std::vector<std::string> Files =
        core::expandGlobPatterns(patterns, projectRoot, core::defaultGlobMatcher(), metadataCache);

    sol::table files = luaState->create_table();
    for (std::size_t index = 0; index < Files.size(); ++index)
    {
        files.set(static_cast<int>(index + 1), Files.at(index));
    }

    return files;
}

}  // namespace beez::plugin::lua::fs_detail
// NOLINTEND(misc-include-cleaner)
