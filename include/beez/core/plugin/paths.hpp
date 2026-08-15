#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace beez::core
{

[[nodiscard]] std::filesystem::path beezCacheDirectory();

[[nodiscard]] std::filesystem::path beezPluginRoot();

[[nodiscard]] std::optional<std::filesystem::path> findPluginScript(const std::string& name,
                                                                    const std::string& version);

[[nodiscard]] std::optional<std::filesystem::path> findPluginScript(const std::string& organization,
                                                                    const std::string& name,
                                                                    const std::string& version);

}  // namespace beez::core
