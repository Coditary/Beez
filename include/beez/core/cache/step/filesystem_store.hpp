#pragma once

#include "beez/core/cache/step/types.hpp"
#include "beez/core/config/cache_options.hpp"

#include <memory>
#include <optional>
#include <string>

namespace beez::core
{

class ICacheStore
{
  public:
    ICacheStore() = default;
    virtual ~ICacheStore() = default;

    ICacheStore(const ICacheStore&) = delete;
    ICacheStore& operator=(const ICacheStore&) = delete;
    ICacheStore(ICacheStore&&) = delete;
    ICacheStore& operator=(ICacheStore&&) = delete;

    [[nodiscard]] virtual std::optional<CacheEntry> lookup(const std::string& key) const = 0;
    virtual void store(const CacheEntry& entry) const = 0;
};

[[nodiscard]] std::unique_ptr<ICacheStore> makeFileSystemCacheStore(const CacheOptions& options);

}  // namespace beez::core
