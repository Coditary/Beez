#include "beez/cli/commands/cache.hpp"

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/cache/storage/migration.hpp"
#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/runtime/context.hpp"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <system_error>

namespace beez::cli
{

void runCacheMaintenance(const ParsedOptions& options,
                         core::BeezSettings& settings,
                         const core::Context& context)
{
    if (options.cleanCache)
    {
        const auto CachePath = settings.resolveCacheOptions(context).root;
        std::error_code errorCode;
        std::filesystem::remove_all(CachePath, errorCode);
        std::cout << "Removed Beez cache: " << CachePath << '\n';
    }

    if (options.updateCache)
    {
        const auto CacheOptions = settings.resolveCacheOptions(context);
        const std::size_t MigratedFiles = core::updateCacheStorage(CacheOptions);
        std::cout << "Updated Beez cache: " << CacheOptions.root << " (";
        std::cout << MigratedFiles << " file";
        if (MigratedFiles != 1U)
        {
            std::cout << 's';
        }
        std::cout << " recompressed to " << core::toString(CacheOptions.compress.algorithm)
                  << ", level " << CacheOptions.compress.level << ", mode "
                  << core::toString(CacheOptions.compress.mode) << ")\n";
    }
}

}  // namespace beez::cli
