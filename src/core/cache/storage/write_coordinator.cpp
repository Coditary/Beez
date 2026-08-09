#include "beez/core/cache/storage/write_coordinator.hpp"

#include "beez/core/cache/storage/envelope.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/config/performance_options.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace beez::core
{

namespace
{

void writeCacheFileImmediate(const std::filesystem::path& path,
                             const std::string& content,
                             const CacheOptions& options)
{
    CacheOptions directOptions = options;
    directOptions.writeCoordinator = nullptr;
    writeCacheFile(path, content, directOptions);
}

}  // namespace

CacheWriteCoordinator::CacheWriteCoordinator(CacheWriteStrategy strategy) : strategy_(strategy) {}

void CacheWriteCoordinator::submit(const std::filesystem::path& path,
                                   const std::string& content,
                                   const CacheOptions& options)
{
    if (strategy_ == CacheWriteStrategy::Immediate)
    {
        writeCacheFileImmediate(path, content, options);
        return;
    }

    const std::scoped_lock Lock(mutex_);
    pending_[path] = content;
}

void CacheWriteCoordinator::flush(const CacheOptions& options)
{
    std::unordered_map<std::filesystem::path, std::string> pending;
    {
        const std::scoped_lock Lock(mutex_);
        pending.swap(pending_);
    }

    for (const auto& [path, content] : pending)
    {
        writeCacheFileImmediate(path, content, options);
    }
}

}  // namespace beez::core
