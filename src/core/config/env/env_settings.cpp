#include "beez/core/config/env/env_settings.hpp"

#include "beez/core/env/env_file.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] const std::vector<std::string>& defaultHashVars()
{
    static const std::vector<std::string> Vars = {
        "CC",
        "CXX",
        "CFLAGS",
        "CXXFLAGS",
        "LDFLAGS",
        "BUILD_TYPE",
    };
    return Vars;
}

[[nodiscard]] const std::vector<std::string>& defaultIgnoreVarsForHashing()
{
    static const std::vector<std::string> Vars = {
        "TERM",
        "COLORTERM",
        "PWD",
        "SESSION_ID",
    };
    return Vars;
}

[[nodiscard]] const std::vector<std::string>& defaultMaskSecrets()
{
    static const std::vector<std::string> Vars = {
        "AWS_ACCESS_KEY_ID",
        "AWS_SECRET_ACCESS_KEY",
        "GITHUB_TOKEN",
        "NPM_AUTH_TOKEN",
    };
    return Vars;
}

[[nodiscard]] std::filesystem::path resolveEnvFilePath(const std::filesystem::path& projectRoot,
                                                       const std::filesystem::path& path)
{
    if (path.is_absolute())
    {
        return path;
    }

    return projectRoot / path;
}

void applyEnvEntries(const std::unordered_map<std::string, std::string>& entries,
                     bool overridesSystem)
{
    for (const auto& [key, value] : entries)
    {
        if (!overridesSystem)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
            if (std::getenv(key.c_str()) != nullptr)
            {
                continue;
            }
        }

        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,misc-include-cleaner)
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void appendResolvedEnvFilePath(const std::filesystem::path& projectRoot,
                               const std::filesystem::path& path,
                               std::unordered_set<std::string>& seen,
                               std::vector<std::filesystem::path>& paths)
{
    const auto Resolved = resolveEnvFilePath(projectRoot, path);
    const std::string Key = Resolved.lexically_normal().string();
    if (seen.insert(Key).second)
    {
        paths.push_back(Resolved);
    }
}

}  // namespace

void mergeEnvSettingsOverlay(EnvSettingsOverlay& base, const EnvSettingsOverlay& overlay)
{
    if (overlay.loadDotenv.has_value())
    {
        base.loadDotenv = overlay.loadDotenv;
    }

    if (overlay.dotenvOverridesSystem.has_value())
    {
        base.dotenvOverridesSystem = overlay.dotenvOverridesSystem;
    }

    base.files.insert(base.files.end(), overlay.files.begin(), overlay.files.end());

    for (const auto& [key, value] : overlay.vars)
    {
        base.vars[key] = value;
    }

    if (!overlay.hashVars.empty())
    {
        base.hashVars = overlay.hashVars;
    }

    if (!overlay.ignoreVarsForHashing.empty())
    {
        base.ignoreVarsForHashing = overlay.ignoreVarsForHashing;
    }

    if (!overlay.maskSecrets.empty())
    {
        base.maskSecrets = overlay.maskSecrets;
    }
}

EnvSettings resolveEnvSettings(const EnvSettingsOverlay& overlay)
{
    EnvSettings resolved;
    resolved.loadDotenv = overlay.loadDotenv.value_or(true);
    resolved.dotenvOverridesSystem = overlay.dotenvOverridesSystem.value_or(false);
    resolved.files = overlay.files;
    resolved.vars = overlay.vars;
    resolved.hashVars = overlay.hashVars.empty() ? defaultHashVars() : overlay.hashVars;
    resolved.ignoreVarsForHashing = overlay.ignoreVarsForHashing.empty()
                                        ? defaultIgnoreVarsForHashing()
                                        : overlay.ignoreVarsForHashing;
    resolved.maskSecrets = overlay.maskSecrets.empty() ? defaultMaskSecrets() : overlay.maskSecrets;
    return resolved;
}

std::vector<std::filesystem::path> resolveEnvFilePaths(const EnvSettings& env,
                                                       const std::filesystem::path& projectRoot)
{
    std::vector<std::filesystem::path> paths;
    paths.reserve(env.files.size() + 1U);

    std::unordered_set<std::string> seen;
    for (const auto& file : env.files)
    {
        appendResolvedEnvFilePath(projectRoot, file, seen, paths);
    }

    if (env.loadDotenv)
    {
        appendResolvedEnvFilePath(projectRoot, ".env", seen, paths);
    }

    return paths;
}

void applyEnvSettings(const EnvSettings& env, const std::filesystem::path& projectRoot)
{
    for (const auto& file : resolveEnvFilePaths(env, projectRoot))
    {
        const EnvFile EnvFileInstance(file);
        EnvFileInstance.forEachEntry(
            [&](const std::string& key, const std::string& value)
            {
                if (!env.dotenvOverridesSystem)
                {
                    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
                    if (std::getenv(key.c_str()) != nullptr)
                    {
                        return;
                    }
                }

                // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,misc-include-cleaner)
                setenv(key.c_str(), value.c_str(), 1);
            });
    }

    applyEnvEntries(env.vars, true);
}

std::string environmentHashFingerprint(const EnvSettings& env)
{
    std::vector<std::string> keys = env.hashVars;
    std::ranges::sort(keys);

    std::ostringstream stream;
    for (const auto& key : keys)
    {
        if (std::ranges::find(env.ignoreVarsForHashing, key) != env.ignoreVarsForHashing.end())
        {
            continue;
        }

        stream << key << '=';
        if (const auto Value = readProcessEnvironment(key))
        {
            stream << *Value;
        }
        stream << '\0';
    }

    return stream.str();
}

bool shouldMaskEnvSecret(const std::string& key, const EnvSettings& env)
{
    return std::ranges::find(env.maskSecrets, key) != env.maskSecrets.end();
}

}  // namespace beez::core
