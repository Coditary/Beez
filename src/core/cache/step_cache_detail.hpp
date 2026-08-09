#pragma once

#include "beez/core/cache/step_cache.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace beez::core
{
class GlobMetadataCache;
class IContentHasher;
}  // namespace beez::core

namespace beez::core::step_cache_detail
{

[[nodiscard]] bool outputsExist(const std::vector<std::string>& outputs,
                              const std::filesystem::path& projectRoot);

[[nodiscard]] bool stepHasArtifacts(const Step& step);

[[nodiscard]] std::string stepExecutionIdentity(const Step& step);

[[nodiscard]] std::string configFingerprint(const StepConfigPtr& config);

[[nodiscard]] std::string buildScriptFingerprint(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher);

[[nodiscard]] std::string stepCommandFingerprint(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher);

[[nodiscard]] std::vector<std::string> artifactPatternsForInputs(const Step& step);

void addDirectoryFromPattern(const std::string& pattern, std::unordered_set<std::string>& directories);

struct InputStamp
{
    std::string path;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type modified;
};

struct CacheIndexEntry
{
    std::string key;
    std::string command;
    std::string config;
    std::string version;
    double durationSeconds = 0.0;
    std::vector<InputStamp> inputs;
    std::vector<std::string> outputs;
};

[[nodiscard]] std::filesystem::path indexPathForStep(const std::filesystem::path& indexRoot,
                                                     const Step& step);

[[nodiscard]] std::vector<InputStamp> collectInputStamps(const Step& step,
                                                         const std::filesystem::path& projectRoot,
                                                         const IGlobMatcher& matcher,
                                                         GlobMetadataCache* globMetadataCache);

[[nodiscard]] bool inputStampsMatch(const std::vector<InputStamp>& expected,
                                    const std::vector<InputStamp>& actual);

[[nodiscard]] std::optional<CacheIndexEntry> readCacheIndex(const std::filesystem::path& indexPath,
                                                            const CacheOptions& options);

void writeCacheIndex(const std::filesystem::path& indexPath,
                     const CacheIndexEntry& entry,
                     const CacheOptions& options);

}  // namespace beez::core::step_cache_detail
