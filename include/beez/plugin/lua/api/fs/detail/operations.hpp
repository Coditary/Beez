#pragma once

#include "beez/core/runtime/context.hpp"

#include <filesystem>
#include <string>

namespace beez::plugin::lua::fs_detail
{

[[nodiscard]] std::filesystem::path resolvedPath(const core::Context& context,
                                               const std::string& userPath);

void copyPath(const core::Context& context,
              const std::string& sourcePath,
              const std::string& destinationPath,
              bool overwrite);

}  // namespace beez::plugin::lua::fs_detail
