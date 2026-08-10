#include "beez/core/cache/step/filesystem_store.hpp"

#include "beez/core/cache/step/types.hpp"
#include "beez/core/cache/storage/envelope.hpp"
#include "beez/core/config/cache/cache_options.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

class FileSystemCacheStore final : public ICacheStore
{
  public:
    explicit FileSystemCacheStore(CacheOptions options)
        : entriesRoot_(options.root / "entries"), options_(std::move(options))
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

        const std::string Manifest = readCacheFile(ManifestPath, options_);

        CacheEntry entry;
        entry.key = key;
        std::istringstream stream(Manifest);
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
        std::ostringstream stream;
        stream << "step=" << entry.stepName << '\n';
        for (const auto& output : entry.outputs)
        {
            stream << "output=" << output << '\n';
        }
        writeCacheFile(ManifestPath, stream.str(), options_);
    }

  private:
    std::filesystem::path entriesRoot_;
    CacheOptions options_;
};

}  // namespace

std::unique_ptr<ICacheStore> makeFileSystemCacheStore(const CacheOptions& options)
{
    return std::make_unique<FileSystemCacheStore>(options);
}

}  // namespace beez::core
