#include "beez/core/cache/storage/migration.hpp"

#include "beez/core/cache/storage/envelope.hpp"
#include "beez/core/config/cache/cache_options.hpp"
#include "storage_detail.hpp"

#include <cstddef>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace beez::core
{

namespace
{

constexpr std::string_view CompressionMetaFileName = "beez-compress.meta";

[[nodiscard]] std::filesystem::path compressionMetaPath(const std::filesystem::path& cacheRoot)
{
    return cacheRoot / CompressionMetaFileName;
}

void writeCompressionMeta(const std::filesystem::path& metaPath,
                          const CacheCompressionSettings& settings)
{
    std::ostringstream stream;
    stream << "algorithm=" << toString(settings.algorithm) << '\n';
    stream << "level=" << settings.level << '\n';
    stream << "mode=" << toString(settings.mode) << '\n';
    storage_detail::writeBinaryFile(metaPath, stream.str());
}

[[nodiscard]] bool migrateCacheFileCompression(const std::filesystem::path& path,
                                               const CacheOptions& options)
{
    const std::string CurrentOnDisk = storage_detail::readBinaryFile(path);
    const std::string Content = readCacheFile(path, options);
    const std::string Desired = storage_detail::buildCachePayload(
        Content, normalizeCacheCompressionSettings(options.compress));
    if (CurrentOnDisk == Desired)
    {
        return false;
    }

    prepareCacheFileForWrite(path, options.protect);
    storage_detail::writeBinaryFile(path, Desired);
    applyCacheFileProtection(path, options.protect);
    return true;
}

}  // namespace

std::size_t updateCacheStorage(const CacheOptions& options)
{
    if (options.root.empty())
    {
        return 0;
    }

    const auto Target = normalizeCacheCompressionSettings(options.compress);
    const auto MetaPath = compressionMetaPath(options.root);

    std::error_code errorCode;
    if (!std::filesystem::exists(options.root, errorCode))
    {
        std::filesystem::create_directories(options.root, errorCode);
        writeCompressionMeta(MetaPath, Target);
        return 0;
    }

    std::size_t migratedFiles = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(options.root, errorCode))
    {
        if (errorCode)
        {
            throw std::runtime_error("failed to scan cache directory: " + options.root.string());
        }

        if (!entry.is_regular_file(errorCode))
        {
            continue;
        }

        if (entry.path() == MetaPath)
        {
            continue;
        }

        if (migrateCacheFileCompression(entry.path(), options))
        {
            ++migratedFiles;
        }
    }

    writeCompressionMeta(MetaPath, Target);
    return migratedFiles;
}

}  // namespace beez::core
