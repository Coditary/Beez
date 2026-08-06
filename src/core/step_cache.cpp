#include "beez/core/step_cache.hpp"

#include "beez/core/content_hash.hpp"
#include "beez/core/glob_expand.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/version.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] bool outputsExist(const std::vector<std::string>& outputs,
                                const std::filesystem::path& projectRoot)
{
    if (outputs.empty())
    {
        return false;
    }

    return std::ranges::all_of(outputs,
                               [&projectRoot](const std::string& relativePath)
                               { return std::filesystem::exists(projectRoot / relativePath); });
}

[[nodiscard]] bool stepHasArtifacts(const Step& step)
{
    return !step.input.empty() || !step.output.empty() || !step.mutate.empty();
}

[[nodiscard]] std::string stepExecutionIdentity(const Step& step)
{
    std::ostringstream stream;
    stream << step.name << '\0' << step.phase << '\0' << step.scope << '\0';
    if (step.shellRun.has_value())
    {
        stream << *step.shellRun;
    }
    else if (step.hasCallback())
    {
        stream << "<callback>";
    }
    return stream.str();
}

[[nodiscard]] std::string configFingerprint(const StepConfigPtr& config)
{
    if (config == nullptr || config->empty())
    {
        return {};
    }
    return config->cacheFingerprint();
}

[[nodiscard]] std::vector<std::string> artifactPatternsForInputs(const Step& step)
{
    std::vector<std::string> patterns = step.input;
    patterns.insert(patterns.end(), step.mutate.begin(), step.mutate.end());
    return patterns;
}

class ContentAddressedCacheKeyStrategy final : public ICacheKeyStrategy
{
  public:
    ContentAddressedCacheKeyStrategy() : hasher_(makeSha256Hasher()) {}

    [[nodiscard]] std::string computeKey(const Step& step,
                                         const std::filesystem::path& projectRoot,
                                         const StepConfigPtr& config,
                                         const IGlobMatcher& matcher) const override
    {
        const auto InputFiles =
            expandGlobPatterns(artifactPatternsForInputs(step), projectRoot, matcher);

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

        return hasher_->combine({stepExecutionIdentity(step),
                                 fileStream.str(),
                                 configFingerprint(config),
                                 version::VersionString});
    }

  private:
    std::unique_ptr<IContentHasher> hasher_;
};

class FileSystemCacheStore final : public ICacheStore
{
  public:
    explicit FileSystemCacheStore(const std::filesystem::path& cacheRoot)
        : entriesRoot_(cacheRoot / "entries")
    {
        std::filesystem::create_directories(entriesRoot_);
    }

    [[nodiscard]] std::optional<CacheEntry> lookup(const std::string& key) const override
    {
        const auto ManifestPath = entriesRoot_ / (key + ".manifest");
        if (!std::filesystem::exists(ManifestPath))
        {
            return std::nullopt;
        }

        std::ifstream stream(ManifestPath);
        if (!stream.is_open())
        {
            return std::nullopt;
        }

        CacheEntry entry;
        entry.key = key;
        std::string line;
        while (std::getline(stream, line))
        {
            const auto Equals = line.find('=');
            if (Equals == std::string::npos)
            {
                continue;
            }

            const std::string Field = line.substr(0, Equals);
            const std::string Value = line.substr(Equals + 1);
            if (Field == "step")
            {
                entry.stepName = Value;
            }
            else if (Field == "output")
            {
                entry.outputs.push_back(Value);
            }
        }

        if (entry.stepName.empty())
        {
            return std::nullopt;
        }

        return entry;
    }

    void store(const CacheEntry& entry) const override
    {
        const auto ManifestPath = entriesRoot_ / (entry.key + ".manifest");
        std::ofstream stream(ManifestPath, std::ios::trunc);
        stream << "step=" << entry.stepName << '\n';
        for (const auto& output : entry.outputs)
        {
            stream << "output=" << output << '\n';
        }
    }

  private:
    std::filesystem::path entriesRoot_;
};

void addDirectoryFromPattern(const std::string& pattern,
                             std::unordered_set<std::string>& directories)
{
    auto slashIndex = pattern.find('/');
    if (slashIndex == std::string::npos)
    {
        return;
    }
    directories.insert(pattern.substr(0, slashIndex));
}

}  // namespace

bool isStepCacheable(const Step& step)
{
    return stepHasArtifacts(step);
}

OutputTracker::OutputTracker(std::filesystem::path projectRoot, const IGlobMatcher& matcher)
    : projectRoot_(std::move(projectRoot)), matcher_(matcher)
{
}

void OutputTracker::begin(const Step& step)
{
    snapshotBefore_ = snapshot(watchDirectories(step));
}

std::vector<std::string> OutputTracker::end(const Step& step)
{
    const auto After = snapshot(watchDirectories(step));
    std::vector<std::string> changed;
    for (const auto& [path, stamp] : After)
    {
        const auto Found = snapshotBefore_.find(path);
        if (Found == snapshotBefore_.end() || Found->second.size != stamp.size ||
            Found->second.modified != stamp.modified)
        {
            changed.push_back(path);
        }
    }

    std::ranges::sort(changed);
    return resolveOutputs(step, changed);
}

std::vector<std::filesystem::path> OutputTracker::watchDirectories(const Step& step) const
{
    std::unordered_set<std::string> directories;
    for (const auto& pattern : step.output)
    {
        addDirectoryFromPattern(pattern, directories);
    }

    if (step.output.empty())
    {
        directories.insert("build");
    }

    std::vector<std::filesystem::path> paths;
    paths.reserve(directories.size());
    std::ranges::transform(directories,
                           std::back_inserter(paths),
                           [this](const std::string& directory)
                           { return projectRoot_ / directory; });
    return paths;
}

std::unordered_map<std::string, OutputTracker::FileStamp>
OutputTracker::snapshot(const std::vector<std::filesystem::path>& directories) const
{
    std::unordered_map<std::string, FileStamp> files;
    for (const auto& directory : directories)
    {
        if (!std::filesystem::exists(directory))
        {
            continue;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 directory, std::filesystem::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            const auto Relative = entry.path().lexically_relative(projectRoot_).generic_string();
            if (Relative.empty() || Relative.starts_with("../"))
            {
                continue;
            }

            std::error_code errorCode;
            files.emplace(Relative,
                          FileStamp {.size = entry.file_size(errorCode),
                                     .modified = entry.last_write_time(errorCode)});
        }
    }

    return files;
}

std::vector<std::string>
OutputTracker::resolveOutputs(const Step& step, const std::vector<std::string>& snapshotDiff) const
{
    if (!step.output.empty())
    {
        return expandGlobPatterns(step.output, projectRoot_, matcher_);
    }

    if (!step.mutate.empty())
    {
        return expandGlobPatterns(step.mutate, projectRoot_, matcher_);
    }

    return snapshotDiff;
}

StepCache::StepCache(const std::filesystem::path& cacheRoot, const IGlobMatcher& matcher)
    : StepCache(
          makeContentAddressedCacheKeyStrategy(), makeFileSystemCacheStore(cacheRoot), matcher)
{
}

StepCache::StepCache(std::unique_ptr<ICacheKeyStrategy> keyStrategy,
                     std::unique_ptr<ICacheStore> store,
                     const IGlobMatcher& matcher)
    : keyStrategy_(std::move(keyStrategy)), store_(std::move(store)), matcher_(matcher)
{
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
    const auto Entry = store_->lookup(result.key);
    if (!Entry.has_value())
    {
        return result;
    }

    result.skip = outputsExist(Entry->outputs, projectRoot);
    return result;
}

void StepCache::store(const Step& step,
                      const std::filesystem::path& projectRoot,
                      const StepConfigPtr& config,
                      const std::vector<std::string>& outputs) const
{
    if (!isStepCacheable(step))
    {
        return;
    }

    CacheEntry entry;
    entry.key = keyStrategy_->computeKey(step, projectRoot, config, matcher_);
    entry.stepName = step.name;
    entry.outputs = outputs;
    store_->store(entry);
}

std::unique_ptr<ICacheKeyStrategy> makeContentAddressedCacheKeyStrategy()
{
    return std::make_unique<ContentAddressedCacheKeyStrategy>();
}

std::unique_ptr<ICacheStore> makeFileSystemCacheStore(const std::filesystem::path& cacheRoot)
{
    return std::make_unique<FileSystemCacheStore>(cacheRoot);
}

}  // namespace beez::core
