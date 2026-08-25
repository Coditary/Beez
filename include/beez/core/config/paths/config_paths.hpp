#pragma once

#include <filesystem>

namespace beez::core
{

[[nodiscard]] std::filesystem::path beezConfigDirectory();
[[nodiscard]] std::filesystem::path globalBeezConfigPath();
[[nodiscard]] std::filesystem::path globalBuildScriptPath();
[[nodiscard]] std::filesystem::path profileBeezConfigPath(const std::string& profileName);

}  // namespace beez::core
