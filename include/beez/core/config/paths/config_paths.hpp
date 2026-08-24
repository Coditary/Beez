#pragma once

#include <filesystem>

namespace beez::core
{

[[nodiscard]] std::filesystem::path beezConfigDirectory();
[[nodiscard]] std::filesystem::path globalBeezConfigPath();
[[nodiscard]] std::filesystem::path globalBuildScriptPath();

}  // namespace beez::core
