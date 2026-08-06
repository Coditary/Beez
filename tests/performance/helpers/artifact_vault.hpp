#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace beez::perf
{

struct ArtifactVaultSpec
{
    std::size_t fileCount = 0;
    std::size_t shardCount = 1;
    std::size_t laneCount = 1;
    std::size_t materializeLimit = 512;
};

class ArtifactVault
{
  public:
    ArtifactVault(const std::filesystem::path& workspace, ArtifactVaultSpec spec)
        : root_(workspace / "vault"), spec_(spec)
    {
        buildCatalog();
        materializeSubset();
        writeManifest();
    }

    [[nodiscard]] std::size_t virtualFileCount() const
    {
        return spec_.fileCount;
    }

    [[nodiscard]] std::size_t materializedFileCount() const
    {
        return materializedCount_;
    }

    [[nodiscard]] std::string
    globForShardLane(std::size_t shard, std::size_t lane, const std::string& extension) const
    {
        return "vault/shard_" + pad(shard, spec_.shardCount) + "/lane_" +
               pad(lane, spec_.laneCount) + "/**/*." + extension;
    }

    [[nodiscard]] std::string globForShard(std::size_t shard, const std::string& extension) const
    {
        return "vault/shard_" + pad(shard, spec_.shardCount) + "/**/*." + extension;
    }

    [[nodiscard]] std::string globForPass(std::size_t shard,
                                          std::size_t lane,
                                          std::size_t pass,
                                          const std::string& extension) const
    {
        return "vault/shard_" + pad(shard, spec_.shardCount) + "/lane_" +
               pad(lane, spec_.laneCount) + "/pass_" + pad(pass, spec_.fileCount + 1U) + "/**/*." +
               extension;
    }

  private:
    [[nodiscard]] static std::string pad(std::size_t value, std::size_t modulus)
    {
        const std::size_t Width = std::to_string(std::max<std::size_t>(modulus, 1U) - 1).size() + 1;
        const auto Text = std::to_string(value);
        if (Text.size() >= Width)
        {
            return Text;
        }
        return std::string(Width - Text.size(), '0') + Text;
    }

    [[nodiscard]] static std::string extensionForStage(std::size_t stage)
    {
        static const std::vector<std::string> Extensions = {
            "raw", "lint", "fmt", "obj", "pak", "tex"};
        return Extensions[stage % Extensions.size()];
    }

    [[nodiscard]] std::string relativePathForIndex(std::size_t index) const
    {
        const std::size_t Shard = index % spec_.shardCount;
        const std::size_t Lane = (index / spec_.shardCount) % spec_.laneCount;
        const std::size_t Ordinal = index / (spec_.shardCount * spec_.laneCount);
        return "vault/shard_" + pad(Shard, spec_.shardCount) + "/lane_" +
               pad(Lane, spec_.laneCount) + "/item_" + pad(Ordinal, spec_.fileCount) + "." +
               extensionForStage(Ordinal % 6);
    }

    void buildCatalog()
    {
        constexpr std::size_t MaxStoredPaths = 10000;
        if (spec_.fileCount > MaxStoredPaths)
        {
            return;
        }

        virtualPaths_.reserve(spec_.fileCount);
        for (std::size_t index = 0; index < spec_.fileCount; ++index)
        {
            virtualPaths_.push_back(relativePathForIndex(index));
        }
    }

    void materializeSubset()
    {
        const std::size_t Limit = std::min(spec_.fileCount, spec_.materializeLimit);
        for (std::size_t index = 0; index < Limit; ++index)
        {
            const std::string RelativePath =
                virtualPaths_.empty() ? relativePathForIndex(index) : virtualPaths_[index];
            const auto FilePath = root_ / RelativePath.substr(std::string("vault/").size());
            std::error_code errorCode;
            std::filesystem::create_directories(FilePath.parent_path(), errorCode);
            std::ofstream file(FilePath);
            if (file.is_open())
            {
                file << "perf\n";
                ++materializedCount_;
            }
        }
    }

    void writeManifest() const
    {
        const auto ManifestPath = root_.parent_path() / "manifest.tsv";
        std::ofstream manifest(ManifestPath);
        if (!manifest.is_open())
        {
            return;
        }

        manifest << "path\n";
        if (virtualPaths_.empty())
        {
            manifest << "(virtual catalog: " << spec_.fileCount << " paths, generated on demand)\n";
            const std::size_t PreviewCount = std::min<std::size_t>(spec_.fileCount, 8);
            for (std::size_t index = 0; index < PreviewCount; ++index)
            {
                manifest << relativePathForIndex(index) << '\n';
            }
            if (spec_.fileCount > PreviewCount)
            {
                manifest << "... (" << (spec_.fileCount - PreviewCount) << " more virtual paths)\n";
            }
            return;
        }

        const std::size_t PreviewCount = std::min<std::size_t>(virtualPaths_.size(), 32);
        for (std::size_t index = 0; index < PreviewCount; ++index)
        {
            manifest << virtualPaths_[index] << '\n';
        }

        if (virtualPaths_.size() > PreviewCount)
        {
            manifest << "... (" << (virtualPaths_.size() - PreviewCount)
                     << " more virtual paths)\n";
        }
    }

    std::filesystem::path root_;
    ArtifactVaultSpec spec_;
    std::vector<std::string> virtualPaths_;
    std::size_t materializedCount_ = 0;
};

}  // namespace beez::perf
