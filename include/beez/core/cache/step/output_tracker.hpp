#pragma once

#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace beez::core
{

class GlobMetadataCache;

[[nodiscard]] bool isStepCacheable(const Step& step);

class OutputTracker
{
  public:
    OutputTracker(std::filesystem::path projectRoot,
                  const IGlobMatcher& matcher,
                  GlobMetadataCache* globMetadataCache = nullptr);

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

    [[nodiscard]] static bool hasExplicitArtifactPatterns(const Step& step);
    [[nodiscard]] std::vector<std::filesystem::path> watchDirectories(const Step& step) const;
    [[nodiscard]] std::unordered_map<std::string, FileStamp>
    snapshotDirectories(const std::vector<std::filesystem::path>& directories) const;

    std::filesystem::path projectRoot_;
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed matcher
    const IGlobMatcher& matcher_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
    GlobMetadataCache* globMetadataCache_ = nullptr;
    std::unordered_map<std::string, FileStamp> snapshotBefore_;
};

}  // namespace beez::core
