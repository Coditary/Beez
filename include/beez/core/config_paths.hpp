#pragma once

#include <filesystem>

namespace beez::core
{

[[nodiscard]] std::filesystem::path beezConfigDirectory();
[[nodiscard]] std::filesystem::path globalBeezConfigPath();

[[nodiscard]] std::filesystem::path
resolveProjectRelativePath(const std::filesystem::path& projectRoot,
                           const std::filesystem::path& path);

}  // namespace beez::core
