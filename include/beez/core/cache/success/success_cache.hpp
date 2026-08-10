#pragma once

#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace beez::core
{

struct StepIdentity
{
    std::string name;
    std::string phase;
    std::string scope;
};

class SuccessCacheSession
{
  public:
    SuccessCacheSession(StepIdentity identity,
                        std::filesystem::path projectRoot,
                        StepConfigPtr config,
                        std::filesystem::path successRoot,
                        CacheOptions cacheOptions);

    [[nodiscard]] bool successCached(const std::string& key) const;
    [[nodiscard]] bool fileSuccessCached(const std::filesystem::path& relativePath) const;
    [[nodiscard]] double fileSavedDurationSeconds(const std::filesystem::path& relativePath) const;

    void cacheSuccess(const std::string& key);
    void cacheFileSuccess(const std::filesystem::path& relativePath, double durationSeconds = 0.0);

    void recordCacheMiss(const std::string& key);
    void recordFileCacheMiss(const std::filesystem::path& relativePath);

    [[nodiscard]] const std::vector<std::string>& getCacheMisses() const;

    void finish();

  private:
    [[nodiscard]] std::string configFingerprint() const;
    [[nodiscard]] std::string entryKey(const std::string& kind, const std::string& value) const;
    [[nodiscard]] std::filesystem::path missesPath() const;
    [[nodiscard]] std::filesystem::path entryManifestPath(const std::string& key) const;

    StepIdentity identity_;
    std::filesystem::path projectRoot_;
    StepConfigPtr config_;
    std::filesystem::path successRoot_;
    CacheOptions cacheOptions_;
    std::vector<std::string> previousMisses_;
    std::vector<std::string> currentMisses_;
};

class SuccessCache
{
  public:
    SuccessCache(const CacheOptions& options, const IGlobMatcher& matcher);

    [[nodiscard]] SuccessCacheSession openSession(const StepIdentity& identity,
                                                  const std::filesystem::path& projectRoot,
                                                  const StepConfigPtr& config) const;

  private:
    std::filesystem::path successRoot_;
    CacheOptions cacheOptions_;
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const IGlobMatcher& matcher_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace beez::core
