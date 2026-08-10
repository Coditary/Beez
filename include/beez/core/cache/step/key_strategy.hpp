#pragma once

#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace beez::core
{

class GlobMetadataCache;

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

[[nodiscard]] std::unique_ptr<ICacheKeyStrategy>
makeContentAddressedCacheKeyStrategy(const ContentHashSettings& hashSettings,
                                     const std::string& envHashFingerprint = {},
                                     GlobMetadataCache* globMetadataCache = nullptr);

}  // namespace beez::core
