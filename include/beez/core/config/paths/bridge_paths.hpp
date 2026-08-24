#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace beez::core
{

struct BridgeLinkResult
{
    std::filesystem::path bridgeDir;
    bool alreadyExisted = false;
};

[[nodiscard]] std::filesystem::path bridgeDirectory();
[[nodiscard]] std::filesystem::path bridgeIndexPath();
[[nodiscard]] std::string hashPath(const std::filesystem::path& path);
[[nodiscard]] std::optional<std::filesystem::path> resolveBridge(const std::filesystem::path& projectRoot);
[[nodiscard]] BridgeLinkResult createBridgeLink(const std::filesystem::path& buildScriptSource,
                                                const std::filesystem::path& projectRoot);

}  // namespace beez::core
