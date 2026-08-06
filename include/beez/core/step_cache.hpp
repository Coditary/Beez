#pragma once

#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace beez::core
{

struct CacheLookupResult
{
    bool skip = false;
    std::string key;
};

struct CacheEntry
{
    std::string key;
    std::string stepName;
    std::vector<std::string> outputs;
};

class ICacheKeyStrategy
{
  public:
    ICacheKeyStrategy() = default;
    virtual ~ICacheKeyStrategy() = default;

    ICacheKeyStrategy(const ICacheKeyStrategy&) = delete;
    ICacheKeyStrategy& operator=(const ICacheKeyStrategy&) = delete;
    ICacheKeyStrategy(ICacheKeyStrategy&&) = delete;
    ICacheKeyStrategy& operator=(ICacheKeyStrategy&&) = delete;

    [[nodiscard]] virtual std::string computeKey(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const StepConfigPtr& config,
                                                 const IGlobMatcher& matcher) const = 0;
};

class ICacheStore
{
  public:
    ICacheStore() = default;
    virtual ~ICacheStore() = default;

    ICacheStore(const ICacheStore&) = delete;
    ICacheStore& operator=(const ICacheStore&) = delete;
    ICacheStore(ICacheStore&&) = delete;
    ICacheStore& operator=(ICacheStore&&) = delete;

    [[nodiscard]] virtual std::optional<CacheEntry> lookup(const std::string& key) const = 0;
    virtual void store(const CacheEntry& entry) const = 0;
};

[[nodiscard]] bool isStepCacheable(const Step& step);

class OutputTracker
{
  public:
    OutputTracker(std::filesystem::path projectRoot, const IGlobMatcher& matcher);

    void begin(const Step& step);
    [[nodiscard]] std::vector<std::string> end(const Step& step);

  private:
    struct FileStamp
    {
        std::uintmax_t size = 0;
        std::filesystem::file_time_type modified;
    };

    [[nodiscard]] std::vector<std::string>
    resolveOutputs(const Step& step, const std::vector<std::string>& snapshotDiff) const;

    [[nodiscard]] std::vector<std::filesystem::path> watchDirectories(const Step& step) const;
    [[nodiscard]] std::unordered_map<std::string, FileStamp>
    snapshot(const std::vector<std::filesystem::path>& directories) const;

    std::filesystem::path projectRoot_;
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed matcher
    const IGlobMatcher& matcher_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::unordered_map<std::string, FileStamp> snapshotBefore_;
};

class StepCache
{
  public:
    StepCache(const std::filesystem::path& cacheRoot, const IGlobMatcher& matcher);
    StepCache(std::unique_ptr<ICacheKeyStrategy> keyStrategy,
              std::unique_ptr<ICacheStore> store,
              const IGlobMatcher& matcher,
              std::filesystem::path indexRoot = {});

    [[nodiscard]] CacheLookupResult lookup(const Step& step,
                                           const std::filesystem::path& projectRoot,
                                           const StepConfigPtr& config) const;
    void store(const Step& step,
               const std::filesystem::path& projectRoot,
               const StepConfigPtr& config,
               const std::vector<std::string>& outputs) const;

    [[nodiscard]] const IGlobMatcher& matcher() const
    {
        return matcher_;
    }

  private:
    std::unique_ptr<ICacheKeyStrategy> keyStrategy_;
    std::unique_ptr<ICacheStore> store_;
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed matcher
    const IGlobMatcher& matcher_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::filesystem::path indexRoot_;

    [[nodiscard]] std::optional<CacheLookupResult>
    lookupViaIndex(const Step& step,
                   const std::filesystem::path& projectRoot,
                   const StepConfigPtr& config) const;

    void writeIndex(const Step& step,
                    const std::filesystem::path& projectRoot,
                    const StepConfigPtr& config,
                    const std::string& key,
                    const std::vector<std::string>& outputs) const;
};

[[nodiscard]] std::unique_ptr<ICacheKeyStrategy> makeContentAddressedCacheKeyStrategy();
[[nodiscard]] std::unique_ptr<ICacheStore>
makeFileSystemCacheStore(const std::filesystem::path& cacheRoot);

}  // namespace beez::core
