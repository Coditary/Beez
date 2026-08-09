#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace beez::core
{

class GlobMetadataCache
{
  public:
    explicit GlobMetadataCache(bool enabled = true);

    void clear();

    [[nodiscard]] std::optional<std::vector<std::string>>
    lookup(const std::string& pattern, const std::filesystem::path& projectRoot) const;

    void store(const std::string& pattern,
               const std::filesystem::path& projectRoot,
               std::vector<std::string> matches);

    [[nodiscard]] bool enabled() const
    {
        return enabled_;
    }

  private:
    [[nodiscard]] static std::string cacheKey(const std::string& pattern,
                                              const std::filesystem::path& projectRoot);

    bool enabled_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::string>> entries_;
};

}  // namespace beez::core
