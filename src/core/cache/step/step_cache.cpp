#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/step/types.hpp"

#include "beez/core/cache/fingerprint/content_hash.hpp"
#include "beez/core/cache/step/filesystem_store.hpp"
#include "beez/core/cache/step/key_strategy.hpp"
#include "beez/core/cache/step/output_tracker.hpp"
#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/version.hpp"
#include "index.hpp"
#include "step_fingerprint.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

StepCache::StepCache(const CacheOptions& options,
                     const IGlobMatcher& matcher,
                     GlobMetadataCache* globMetadataCache)
    : StepCache(makeContentAddressedCacheKeyStrategy(
                    options.hash, options.envHashFingerprint, globMetadataCache),
                makeFileSystemCacheStore(options),
                matcher,
                options.root / "index",
                options,
                globMetadataCache)
{
}

StepCache::StepCache(std::unique_ptr<ICacheKeyStrategy> keyStrategy,
                     std::unique_ptr<ICacheStore> store,
                     const IGlobMatcher& matcher,
                     std::filesystem::path indexRoot,
                     CacheOptions cacheOptions,
                     GlobMetadataCache* globMetadataCache)
    : keyStrategy_(std::move(keyStrategy)), store_(std::move(store)), matcher_(matcher),
      indexRoot_(std::move(indexRoot)), cacheOptions_(std::move(cacheOptions)),
      globMetadataCache_(globMetadataCache)
{
    if (!indexRoot_.empty())
    {
        std::filesystem::create_directories(indexRoot_);
    }
}

CacheLookupResult StepCache::lookup(const Step& step,
                                    const std::filesystem::path& projectRoot,
                                    const StepConfigPtr& config) const
{
    CacheLookupResult result;
    if (!isStepCacheable(step))
    {
        return result;
    }

    result.key = keyStrategy_->computeKey(step, projectRoot, config, matcher_);

    if (!indexRoot_.empty())
    {
        if (const auto Indexed = lookupViaIndex(step, projectRoot, config))
        {
            if (Indexed->skip && store_->lookup(Indexed->key).has_value())
            {
                return *Indexed;
            }
        }
    }

    const auto Entry = store_->lookup(result.key);
    if (!Entry.has_value())
    {
        return result;
    }

    result.skip = step_cache_detail::outputsMatch(
        Entry->outputs, Entry->outputHashes, projectRoot, *makeContentHasher(cacheOptions_.hash));
    if (result.skip && !indexRoot_.empty())
    {
        const auto IndexPath = step_cache_detail::indexPathForStep(indexRoot_, step);
        const auto ExistingIndex = step_cache_detail::readCacheIndex(IndexPath, cacheOptions_);
        const double PreservedDuration =
            ExistingIndex.has_value() ? ExistingIndex->durationSeconds : 0.0;
        writeIndex(step, projectRoot, config, result.key, Entry->outputs, PreservedDuration);
    }

    return result;
}

void StepCache::store(const Step& step,
                      const std::filesystem::path& projectRoot,
                      const StepConfigPtr& config,
                      const std::vector<std::string>& outputs,
                      const double DurationSeconds) const
{
    if (!isStepCacheable(step))
    {
        return;
    }

    CacheEntry entry;
    entry.key = keyStrategy_->computeKey(step, projectRoot, config, matcher_);
    entry.stepName = step.name;
    entry.outputs = outputs;
    entry.outputHashes = step_cache_detail::computeOutputHashes(
        outputs, projectRoot, *makeContentHasher(cacheOptions_.hash));
    store_->store(entry);

    if (!indexRoot_.empty())
    {
        writeIndex(step, projectRoot, config, entry.key, outputs, DurationSeconds);
    }
}

std::optional<CacheLookupResult> StepCache::lookupViaIndex(const Step& step,
                                                           const std::filesystem::path& projectRoot,
                                                           const StepConfigPtr& config) const
{
    const auto IndexPath = step_cache_detail::indexPathForStep(indexRoot_, step);
    const auto IndexEntry = step_cache_detail::readCacheIndex(IndexPath, cacheOptions_);
    if (!IndexEntry.has_value())
    {
        return std::nullopt;
    }

    if (IndexEntry->command != step_cache_detail::stepCommandFingerprint(
                                   step, projectRoot, *makeContentHasher(cacheOptions_.hash)) ||
        IndexEntry->config != step_cache_detail::configFingerprint(config) ||
        IndexEntry->version != version::VersionString)
    {
        return std::nullopt;
    }

    const auto CurrentStamps =
        step_cache_detail::collectInputStamps(step, projectRoot, matcher_, globMetadataCache_);
    if (!step_cache_detail::inputStampsMatch(IndexEntry->inputs, CurrentStamps))
    {
        return std::nullopt;
    }

    if (!step_cache_detail::outputsMatch(IndexEntry->outputs,
                                         IndexEntry->outputHashes,
                                         projectRoot,
                                         *makeContentHasher(cacheOptions_.hash)))
    {
        return std::nullopt;
    }

    CacheLookupResult result;
    result.key = IndexEntry->key;
    result.skip = true;
    result.savedDurationSeconds = IndexEntry->durationSeconds;
    return result;
}

void StepCache::writeIndex(const Step& step,
                           const std::filesystem::path& projectRoot,
                           const StepConfigPtr& config,
                           const std::string& key,
                           const std::vector<std::string>& outputs,
                           const double DurationSeconds) const
{
    step_cache_detail::CacheIndexEntry indexEntry;
    indexEntry.key = key;
    indexEntry.command = step_cache_detail::stepCommandFingerprint(
        step, projectRoot, *makeContentHasher(cacheOptions_.hash));
    indexEntry.config = step_cache_detail::configFingerprint(config);
    indexEntry.version = version::VersionString;
    indexEntry.durationSeconds = DurationSeconds;
    indexEntry.inputs =
        step_cache_detail::collectInputStamps(step, projectRoot, matcher_, globMetadataCache_);
    indexEntry.outputs = outputs;
    indexEntry.outputHashes = step_cache_detail::computeOutputHashes(
        outputs, projectRoot, *makeContentHasher(cacheOptions_.hash));
    step_cache_detail::writeCacheIndex(
        step_cache_detail::indexPathForStep(indexRoot_, step), indexEntry, cacheOptions_);
}

}  // namespace beez::core
