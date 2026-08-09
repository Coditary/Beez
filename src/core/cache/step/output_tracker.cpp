#include "beez/core/cache/step/output_tracker.hpp"

#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "index.hpp"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <ranges>  // NOLINT(misc-include-cleaner) -- std::ranges::sort, std::ranges::transform
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace beez::core
{

bool isStepCacheable(const Step& step)
{
    return step_cache_detail::stepHasArtifacts(step);
}

OutputTracker::OutputTracker(std::filesystem::path projectRoot,
                             const IGlobMatcher& matcher,
                             GlobMetadataCache* globMetadataCache)
    : projectRoot_(std::move(projectRoot)), matcher_(matcher), globMetadataCache_(globMetadataCache)
{
}

bool OutputTracker::hasExplicitArtifactPatterns(const Step& step)
{
    return !step.output.empty() || !step.mutate.empty();
}

void OutputTracker::begin(const Step& step)
{
    if (hasExplicitArtifactPatterns(step))
    {
        return;
    }

    snapshotBefore_ = snapshotDirectories(watchDirectories(step));
}

std::vector<std::string> OutputTracker::end(const Step& step)
{
    if (hasExplicitArtifactPatterns(step))
    {
        return resolveOutputs(step, {});
    }

    const auto After = snapshotDirectories(watchDirectories(step));
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
        step_cache_detail::addDirectoryFromPattern(pattern, directories);
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
OutputTracker::snapshotDirectories(const std::vector<std::filesystem::path>& directories) const
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
        return expandGlobPatterns(step.output, projectRoot_, matcher_, globMetadataCache_);
    }

    if (!step.mutate.empty())
    {
        return expandGlobPatterns(step.mutate, projectRoot_, matcher_, globMetadataCache_);
    }

    return snapshotDiff;
}

}  // namespace beez::core
