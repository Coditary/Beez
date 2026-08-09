#include "beez/core/cache/step/key_strategy.hpp"

#include "beez/core/cache/fingerprint/content_hash.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/version.hpp"
#include "index.hpp"
#include "step_fingerprint.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <ranges>  // NOLINT(misc-include-cleaner) -- std::ranges::sort
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

class ContentAddressedCacheKeyStrategy final : public ICacheKeyStrategy
{
  public:
    ContentAddressedCacheKeyStrategy(const ContentHashSettings& settings,
                                     std::string envHashFingerprint,
                                     GlobMetadataCache* globMetadataCache)
        : hasher_(makeContentHasher(settings)), envHashFingerprint_(std::move(envHashFingerprint)),
          globMetadataCache_(globMetadataCache)
    {
    }

    [[nodiscard]] std::string computeKey(const Step& step,
                                         const std::filesystem::path& projectRoot,
                                         const StepConfigPtr& config,
                                         const IGlobMatcher& matcher) const override
    {
        const auto InputFiles =
            expandGlobPatterns(step_cache_detail::artifactPatternsForInputs(step),
                               projectRoot,
                               matcher,
                               globMetadataCache_);

        std::vector<std::string> fileParts;
        fileParts.reserve(InputFiles.size());
        for (const auto& relativePath : InputFiles)
        {
            const auto Absolute = projectRoot / relativePath;
            fileParts.push_back(relativePath + '\0' + hasher_->hashFile(Absolute));
        }

        std::ranges::sort(fileParts);

        std::ostringstream fileStream;
        for (const auto& part : fileParts)
        {
            fileStream << part << '\0';
        }

        return hasher_->combine(
            {step_cache_detail::stepExecutionIdentity(step),
             step_cache_detail::buildScriptFingerprint(step, projectRoot, *hasher_),
             fileStream.str(),
             step_cache_detail::configFingerprint(config),
             envHashFingerprint_,
             version::VersionString});
    }

  private:
    std::unique_ptr<IContentHasher> hasher_;
    std::string envHashFingerprint_;
    GlobMetadataCache* globMetadataCache_ = nullptr;
};

}  // namespace

std::unique_ptr<ICacheKeyStrategy>
makeContentAddressedCacheKeyStrategy(const ContentHashSettings& hashSettings,
                                     const std::string& envHashFingerprint,
                                     GlobMetadataCache* globMetadataCache)
{
    return std::make_unique<ContentAddressedCacheKeyStrategy>(
        hashSettings, envHashFingerprint, globMetadataCache);
}

}  // namespace beez::core
