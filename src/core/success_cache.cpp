#include "beez/core/success_cache.hpp"

#include "beez/core/content_hash.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step_config.hpp"
#include "beez/version.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ranges>  // NOLINT(misc-include-cleaner) -- std::ranges algorithms for container mutation
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] std::string sanitizePathComponent(std::string value)
{
    std::ranges::replace_if(
        value,
        [](const char Character)
        { return Character == '/' || Character == ':' || Character == '\\'; },
        '_');
    return value;
}

[[nodiscard]] std::string configFingerprint(const StepConfigPtr& config)
{
    if (config == nullptr || config->empty())
    {
        return {};
    }
    return config->cacheFingerprint();
}

[[nodiscard]] std::string readManifestField(const std::filesystem::path& manifestPath,
                                            const std::string& fieldName)
{
    std::ifstream stream(manifestPath);
    if (!stream.is_open())
    {
        return {};
    }

    std::string line;
    while (std::getline(stream, line))
    {
        const auto Equals = line.find('=');
        if (Equals == std::string::npos)
        {
            continue;
        }

        if (line.substr(0, Equals) == fieldName)
        {
            return line.substr(Equals + 1);
        }
    }

    return {};
}

[[nodiscard]] std::vector<std::string> loadMissesFile(const std::filesystem::path& missesPath)
{
    if (!std::filesystem::exists(missesPath))
    {
        return {};
    }

    std::ifstream stream(missesPath);
    if (!stream.is_open())
    {
        return {};
    }

    std::vector<std::string> misses;
    std::string line;
    bool pastHeader = false;
    while (std::getline(stream, line))
    {
        if (!pastHeader)
        {
            if (line == "---")
            {
                pastHeader = true;
            }
            continue;
        }

        if (!line.empty())
        {
            misses.push_back(line);
        }
    }

    return misses;
}

[[nodiscard]] std::string hashConfigFingerprint(const std::string& fingerprint)
{
    const auto Hasher = makeSha256Hasher();
    return Hasher->hashBytes(fingerprint);
}

void writeMissesFile(const std::filesystem::path& missesPath,
                     const std::string& config,
                     const std::vector<std::string>& misses)
{
    std::filesystem::create_directories(missesPath.parent_path());
    std::ofstream stream(missesPath, std::ios::trunc);
    stream << "config_hash=" << hashConfigFingerprint(config) << '\n';
    stream << "version=" << version::VersionString << '\n';
    stream << "---\n";
    for (const auto& miss : misses)
    {
        stream << miss << '\n';
    }
}

[[nodiscard]] bool missesHeaderMatches(const std::filesystem::path& missesPath,
                                       const std::string& config)
{
    if (!std::filesystem::exists(missesPath))
    {
        return true;
    }

    const std::string ConfigHash = hashConfigFingerprint(config);
    const std::string StoredConfigHash = readManifestField(missesPath, "config_hash");
    const std::string StoredVersion = readManifestField(missesPath, "version");
    return StoredConfigHash == ConfigHash && StoredVersion == version::VersionString;
}

[[nodiscard]] std::vector<std::string> normalizeMisses(std::vector<std::string> misses)
{
    std::ranges::sort(misses);
    const auto [begin, end] = std::ranges::unique(misses);
    misses.erase(begin, end);
    return misses;
}

void removeMiss(std::vector<std::string>& misses, const std::string& key)
{
    const auto Found = std::ranges::find(misses, key);
    if (Found != misses.end())
    {
        misses.erase(Found);
    }
}

void addMiss(std::vector<std::string>& misses, const std::string& key)
{
    if (std::ranges::find(misses, key) == misses.end())
    {
        misses.push_back(key);
    }
}

}  // namespace

SuccessCacheSession::SuccessCacheSession(StepIdentity identity,
                                         std::filesystem::path projectRoot,
                                         StepConfigPtr config,
                                         std::filesystem::path successRoot)
    : identity_(std::move(identity)), projectRoot_(std::move(projectRoot)),
      config_(std::move(config)), successRoot_(std::move(successRoot))
{
    const auto MissesPath = missesPath();
    if (missesHeaderMatches(MissesPath, ::beez::core::configFingerprint(config_)))
    {
        previousMisses_ = loadMissesFile(MissesPath);
    }
    currentMisses_ = previousMisses_;
}

std::string SuccessCacheSession::configFingerprint() const
{
    return ::beez::core::configFingerprint(config_);
}

std::filesystem::path SuccessCacheSession::missesPath() const
{
    const std::string FileName = sanitizePathComponent(identity_.name) + "__" +
                                 sanitizePathComponent(identity_.phase) + "__" +
                                 sanitizePathComponent(identity_.scope) + ".misses";
    return successRoot_ / "misses" / FileName;
}

std::string SuccessCacheSession::entryKey(const std::string& kind, const std::string& value) const
{
    const auto Hasher = makeSha256Hasher();
    return Hasher->combine({identity_.name,
                            identity_.phase,
                            identity_.scope,
                            configFingerprint(),
                            version::VersionString,
                            kind,
                            value});
}

std::filesystem::path SuccessCacheSession::entryManifestPath(const std::string& key) const
{
    return successRoot_ / "entries" / (key + ".manifest");
}

bool SuccessCacheSession::entryMatchesCurrentContext(
    const std::filesystem::path& manifestPath) const
{
    if (!std::filesystem::exists(manifestPath))
    {
        return false;
    }

    const std::string StoredStep = readManifestField(manifestPath, "step");
    const std::string StoredPhase = readManifestField(manifestPath, "phase");
    const std::string StoredScope = readManifestField(manifestPath, "scope");
    const std::string StoredConfigHash = readManifestField(manifestPath, "config_hash");
    const std::string StoredVersion = readManifestField(manifestPath, "version");

    return StoredStep == identity_.name && StoredPhase == identity_.phase &&
           StoredScope == identity_.scope &&
           StoredConfigHash == hashConfigFingerprint(configFingerprint()) &&
           StoredVersion == version::VersionString;
}

bool SuccessCacheSession::successCached(const std::string& key) const
{
    const auto ManifestPath = entryManifestPath(entryKey("string", key));
    if (!entryMatchesCurrentContext(ManifestPath))
    {
        return false;
    }

    return readManifestField(ManifestPath, "kind") == "string" &&
           readManifestField(ManifestPath, "key") == key;
}

bool SuccessCacheSession::fileSuccessCached(const std::filesystem::path& relativePath) const
{
    const std::string NormalizedPath = relativePath.generic_string();
    const auto ManifestPath = entryManifestPath(entryKey("file", NormalizedPath));
    if (!entryMatchesCurrentContext(ManifestPath))
    {
        return false;
    }

    if (readManifestField(ManifestPath, "kind") != "file" ||
        readManifestField(ManifestPath, "key") != NormalizedPath)
    {
        return false;
    }

    const auto Absolute = projectRoot_ / relativePath;
    std::error_code errorCode;
    if (!std::filesystem::is_regular_file(Absolute, errorCode))
    {
        return false;
    }

    const auto Hasher = makeSha256Hasher();
    const std::string CurrentHash = Hasher->hashFile(Absolute);
    return readManifestField(ManifestPath, "file_hash") == CurrentHash;
}

void SuccessCacheSession::cacheSuccess(const std::string& key)
{
    const auto Key = entryKey("string", key);
    const auto ManifestPath = entryManifestPath(Key);
    std::filesystem::create_directories(ManifestPath.parent_path());

    std::ofstream stream(ManifestPath, std::ios::trunc);
    stream << "step=" << identity_.name << '\n';
    stream << "phase=" << identity_.phase << '\n';
    stream << "scope=" << identity_.scope << '\n';
    stream << "config_hash=" << hashConfigFingerprint(configFingerprint()) << '\n';
    stream << "version=" << version::VersionString << '\n';
    stream << "kind=string\n";
    stream << "key=" << key << '\n';

    removeMiss(currentMisses_, key);
}

void SuccessCacheSession::cacheFileSuccess(const std::filesystem::path& relativePath)
{
    const std::string NormalizedPath = relativePath.generic_string();
    const auto Absolute = projectRoot_ / relativePath;
    const auto Hasher = makeSha256Hasher();
    const std::string FileHash = Hasher->hashFile(Absolute);

    const auto Key = entryKey("file", NormalizedPath);
    const auto ManifestPath = entryManifestPath(Key);
    std::filesystem::create_directories(ManifestPath.parent_path());

    std::ofstream stream(ManifestPath, std::ios::trunc);
    stream << "step=" << identity_.name << '\n';
    stream << "phase=" << identity_.phase << '\n';
    stream << "scope=" << identity_.scope << '\n';
    stream << "config_hash=" << hashConfigFingerprint(configFingerprint()) << '\n';
    stream << "version=" << version::VersionString << '\n';
    stream << "kind=file\n";
    stream << "key=" << NormalizedPath << '\n';
    stream << "file_hash=" << FileHash << '\n';

    removeMiss(currentMisses_, NormalizedPath);
}

void SuccessCacheSession::recordCacheMiss(const std::string& key)
{
    addMiss(currentMisses_, key);
}

void SuccessCacheSession::recordFileCacheMiss(const std::filesystem::path& relativePath)
{
    addMiss(currentMisses_, relativePath.generic_string());
}

const std::vector<std::string>& SuccessCacheSession::getCacheMisses() const
{
    return previousMisses_;
}

void SuccessCacheSession::finish()
{
    writeMissesFile(missesPath(), configFingerprint(), normalizeMisses(currentMisses_));
}

SuccessCache::SuccessCache(const std::filesystem::path& cacheRoot, const IGlobMatcher& matcher)
    : successRoot_(cacheRoot / "success"), matcher_(matcher)
{
    std::filesystem::create_directories(successRoot_);
}

SuccessCacheSession SuccessCache::openSession(const StepIdentity& identity,
                                              const std::filesystem::path& projectRoot,
                                              const StepConfigPtr& config) const
{
    return {identity, projectRoot, config, successRoot_};
}

}  // namespace beez::core
