#include "beez/core/success_cache.hpp"

#include "beez/core/cache_options.hpp"
#include "beez/core/cache_storage.hpp"
#include "beez/core/content_hash.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/include_fingerprint.hpp"
#include "beez/core/step_config.hpp"
#include "beez/version.hpp"

#include <algorithm>
#include <filesystem>
#include <ranges>  // NOLINT(misc-include-cleaner) -- std::ranges algorithms for container mutation
#include <sstream>
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
                                            const std::string& fieldName,
                                            const CacheOptions& cacheOptions)
{
    if (!std::filesystem::exists(manifestPath))
    {
        return {};
    }

    const std::string Manifest = readCacheFile(manifestPath, cacheOptions);
    std::istringstream stream(Manifest);
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

[[nodiscard]] std::vector<std::string> loadMissesFile(const std::filesystem::path& missesPath,
                                                      const CacheOptions& cacheOptions)
{
    if (!std::filesystem::exists(missesPath))
    {
        return {};
    }

    const std::string Manifest = readCacheFile(missesPath, cacheOptions);
    std::istringstream stream(Manifest);
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

[[nodiscard]] std::string hashConfigFingerprint(const std::string& fingerprint,
                                                const ContentHashSettings& hashSettings)
{
    const auto Hasher = makeContentHasher(hashSettings);
    return Hasher->hashBytes(fingerprint);
}

void writeMissesFile(const std::filesystem::path& missesPath,
                     const std::string& config,
                     const std::vector<std::string>& misses,
                     const CacheOptions& cacheOptions)
{
    std::ostringstream stream;
    stream << "config_hash=" << hashConfigFingerprint(config, cacheOptions.hash) << '\n';
    stream << "version=" << version::VersionString << '\n';
    stream << "---\n";
    for (const auto& miss : misses)
    {
        stream << miss << '\n';
    }
    writeCacheFile(missesPath, stream.str(), cacheOptions);
}

[[nodiscard]] bool missesHeaderMatches(const std::filesystem::path& missesPath,
                                       const std::string& config,
                                       const CacheOptions& cacheOptions)
{
    if (!std::filesystem::exists(missesPath))
    {
        return true;
    }

    const std::string Manifest = readCacheFile(missesPath, cacheOptions);
    std::istringstream stream(Manifest);
    std::string line;
    std::string storedConfigHash;
    std::string storedVersion;
    while (std::getline(stream, line))
    {
        if (line == "---")
        {
            break;
        }

        const auto Equals = line.find('=');
        if (Equals == std::string::npos)
        {
            continue;
        }

        const std::string Field = line.substr(0, Equals);
        const std::string Value = line.substr(Equals + 1);
        if (Field == "config_hash")
        {
            storedConfigHash = Value;
        }
        else if (Field == "version")
        {
            storedVersion = Value;
        }
    }

    const std::string ConfigHash = hashConfigFingerprint(config, cacheOptions.hash);
    return storedConfigHash == ConfigHash && storedVersion == version::VersionString;
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
                                         std::filesystem::path successRoot,
                                         CacheOptions cacheOptions)
    : identity_(std::move(identity)), projectRoot_(std::move(projectRoot)),
      config_(std::move(config)), successRoot_(std::move(successRoot)),
      cacheOptions_(std::move(cacheOptions))
{
    const auto MissesPath = missesPath();
    if (missesHeaderMatches(MissesPath, ::beez::core::configFingerprint(config_), cacheOptions_))
    {
        previousMisses_ = loadMissesFile(MissesPath, cacheOptions_);
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
    const auto Hasher = makeContentHasher(cacheOptions_.hash);
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

    const std::string StoredStep = readManifestField(manifestPath, "step", cacheOptions_);
    const std::string StoredPhase = readManifestField(manifestPath, "phase", cacheOptions_);
    const std::string StoredScope = readManifestField(manifestPath, "scope", cacheOptions_);
    const std::string StoredConfigHash =
        readManifestField(manifestPath, "config_hash", cacheOptions_);
    const std::string StoredVersion = readManifestField(manifestPath, "version", cacheOptions_);

    return StoredStep == identity_.name && StoredPhase == identity_.phase &&
           StoredScope == identity_.scope &&
           StoredConfigHash == hashConfigFingerprint(configFingerprint(), cacheOptions_.hash) &&
           StoredVersion == version::VersionString;
}

bool SuccessCacheSession::successCached(const std::string& key) const
{
    const auto ManifestPath = entryManifestPath(entryKey("string", key));
    if (!entryMatchesCurrentContext(ManifestPath))
    {
        return false;
    }

    return readManifestField(ManifestPath, "kind", cacheOptions_) == "string" &&
           readManifestField(ManifestPath, "key", cacheOptions_) == key;
}

bool SuccessCacheSession::fileSuccessCached(const std::filesystem::path& relativePath) const
{
    const std::string NormalizedPath = relativePath.generic_string();
    const auto ManifestPath = entryManifestPath(entryKey("file", NormalizedPath));
    if (!entryMatchesCurrentContext(ManifestPath))
    {
        return false;
    }

    if (readManifestField(ManifestPath, "kind", cacheOptions_) != "file" ||
        readManifestField(ManifestPath, "key", cacheOptions_) != NormalizedPath)
    {
        return false;
    }

    const auto Absolute = projectRoot_ / relativePath;
    std::error_code errorCode;
    if (!std::filesystem::is_regular_file(Absolute, errorCode))
    {
        return false;
    }

    const auto Hasher = makeContentHasher(cacheOptions_.hash);
    const std::string CurrentHash = Hasher->hashFile(Absolute);
    const std::string CurrentInputsHash = includeTreeFingerprint(Absolute, projectRoot_, *Hasher);
    return readManifestField(ManifestPath, "file_hash", cacheOptions_) == CurrentHash &&
           readManifestField(ManifestPath, "inputs_hash", cacheOptions_) == CurrentInputsHash;
}

void SuccessCacheSession::cacheSuccess(const std::string& key)
{
    const auto Key = entryKey("string", key);
    const auto ManifestPath = entryManifestPath(Key);
    std::ostringstream stream;
    stream << "step=" << identity_.name << '\n';
    stream << "phase=" << identity_.phase << '\n';
    stream << "scope=" << identity_.scope << '\n';
    stream << "config_hash=" << hashConfigFingerprint(configFingerprint(), cacheOptions_.hash)
           << '\n';
    stream << "version=" << version::VersionString << '\n';
    stream << "kind=string\n";
    stream << "key=" << key << '\n';
    writeCacheFile(ManifestPath, stream.str(), cacheOptions_);

    removeMiss(currentMisses_, key);
}

void SuccessCacheSession::cacheFileSuccess(const std::filesystem::path& relativePath)
{
    const std::string NormalizedPath = relativePath.generic_string();
    const auto Absolute = projectRoot_ / relativePath;
    const auto Hasher = makeContentHasher(cacheOptions_.hash);
    const std::string FileHash = Hasher->hashFile(Absolute);
    const std::string InputsHash = includeTreeFingerprint(Absolute, projectRoot_, *Hasher);

    const auto Key = entryKey("file", NormalizedPath);
    const auto ManifestPath = entryManifestPath(Key);
    std::ostringstream stream;
    stream << "step=" << identity_.name << '\n';
    stream << "phase=" << identity_.phase << '\n';
    stream << "scope=" << identity_.scope << '\n';
    stream << "config_hash=" << hashConfigFingerprint(configFingerprint(), cacheOptions_.hash)
           << '\n';
    stream << "version=" << version::VersionString << '\n';
    stream << "kind=file\n";
    stream << "key=" << NormalizedPath << '\n';
    stream << "file_hash=" << FileHash << '\n';
    stream << "inputs_hash=" << InputsHash << '\n';
    writeCacheFile(ManifestPath, stream.str(), cacheOptions_);

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
    writeMissesFile(
        missesPath(), configFingerprint(), normalizeMisses(currentMisses_), cacheOptions_);
}

SuccessCache::SuccessCache(const CacheOptions& options, const IGlobMatcher& matcher)
    : successRoot_(options.root / "success"), cacheOptions_(options), matcher_(matcher)
{
    std::filesystem::create_directories(successRoot_);
}

SuccessCacheSession SuccessCache::openSession(const StepIdentity& identity,
                                              const std::filesystem::path& projectRoot,
                                              const StepConfigPtr& config) const
{
    return {identity, projectRoot, config, successRoot_, cacheOptions_};
}

}  // namespace beez::core
