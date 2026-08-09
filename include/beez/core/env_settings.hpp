#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace beez::core
{

struct EnvSettingsOverlay
{
    std::optional<bool> loadDotenv;
    std::optional<bool> dotenvOverridesSystem;
    std::vector<std::filesystem::path> files;
    std::unordered_map<std::string, std::string> vars;
    std::vector<std::string> hashVars;
    std::vector<std::string> ignoreVarsForHashing;
    std::vector<std::string> maskSecrets;
};

struct EnvSettings
{
    bool loadDotenv = true;
    bool dotenvOverridesSystem = false;
    std::vector<std::filesystem::path> files;
    std::unordered_map<std::string, std::string> vars;
    std::vector<std::string> hashVars;
    std::vector<std::string> ignoreVarsForHashing;
    std::vector<std::string> maskSecrets;
};

void mergeEnvSettingsOverlay(EnvSettingsOverlay& base, const EnvSettingsOverlay& overlay);

[[nodiscard]] EnvSettings resolveEnvSettings(const EnvSettingsOverlay& overlay);

[[nodiscard]] std::vector<std::filesystem::path>
resolveEnvFilePaths(const EnvSettings& env, const std::filesystem::path& projectRoot);

void applyEnvSettings(const EnvSettings& env, const std::filesystem::path& projectRoot);

[[nodiscard]] std::string environmentHashFingerprint(const EnvSettings& env);

[[nodiscard]] bool shouldMaskEnvSecret(const std::string& key, const EnvSettings& env);

}  // namespace beez::core
