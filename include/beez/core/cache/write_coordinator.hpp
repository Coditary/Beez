#pragma once

#include "beez/core/config/cache_options.hpp"
#include "beez/core/config/performance_options.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace beez::core
{

class CacheWriteCoordinator
{
  public:
    explicit CacheWriteCoordinator(CacheWriteStrategy strategy = CacheWriteStrategy::Phase);

    void submit(const std::filesystem::path& path,
                const std::string& content,
                const CacheOptions& options);

    void flush(const CacheOptions& options);

    [[nodiscard]] CacheWriteStrategy strategy() const
    {
        return strategy_;
    }

    [[nodiscard]] bool buffersWrites() const
    {
        return strategy_ != CacheWriteStrategy::Immediate;
    }

  private:
    CacheWriteStrategy strategy_;
    std::mutex mutex_;
    std::unordered_map<std::filesystem::path, std::string> pending_;
};

}  // namespace beez::core
