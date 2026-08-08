#pragma once

#include <filesystem>

namespace beez::core
{

[[nodiscard]] std::filesystem::path beezConfigDirectory();
[[nodiscard]] std::filesystem::path globalBeezConfigPath();

}  // namespace beez::core
