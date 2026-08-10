#pragma once

#include "beez/core/cache/step/filesystem_store.hpp"
#include "beez/core/cache/step/key_strategy.hpp"
#include "beez/core/cache/step/types.hpp"
#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

class GlobMetadataCache;

class StepCache
{
  public:
    StepCache(const CacheOptions& options,
              const IGlobMatcher& matcher,
              GlobMetadataCache* globMetadataCache = nullptr);
    StepCache(std::unique_ptr<ICacheKeyStrategy> keyStrategy,
              std::unique_ptr<ICacheStore> store,
              const IGlobMatcher& matcher,
              std::filesystem::path indexRoot = {},
              CacheOptions cacheOptions = {},
              GlobMetadataCache* globMetadataCache = nullptr);

    [[nodiscard]] CacheLookupResult lookup(const Step& step,
                                           const std::filesystem::path& projectRoot,
                                           const StepConfigPtr& config) const;
    void store(const Step& step,
               const std::filesystem::path& projectRoot,
               const StepConfigPtr& config,
               const std::vector<std::string>& outputs,
               double durationSeconds = 0.0) const;

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
    CacheOptions cacheOptions_;
    GlobMetadataCache* globMetadataCache_ = nullptr;

    [[nodiscard]] std::optional<CacheLookupResult>
    lookupViaIndex(const Step& step,
                   const std::filesystem::path& projectRoot,
                   const StepConfigPtr& config) const;

    void writeIndex(const Step& step,
                    const std::filesystem::path& projectRoot,
                    const StepConfigPtr& config,
                    const std::string& key,
                    const std::vector<std::string>& outputs,
                    double durationSeconds = 0.0) const;
};

}  // namespace beez::core
