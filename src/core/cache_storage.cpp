#include "beez/core/cache_storage.hpp"

#include "beez/core/cache_compress.hpp"
#include "beez/core/cache_options.hpp"

#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace beez::core
{

namespace
{

constexpr std::string_view CacheMagic = "BEEZCACHE1\n";
constexpr std::string_view CacheSeparator = "\n---\n";
constexpr std::string_view CompressionMetaFileName = "beez-compress.meta";

[[nodiscard]] std::filesystem::path compressionMetaPath(const std::filesystem::path& cacheRoot)
{
    return cacheRoot / CompressionMetaFileName;
}

[[nodiscard]] std::string readBinaryFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        throw std::runtime_error("failed to read cache file: " + path.string());
    }

    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

void writeBinaryFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open())
    {
        throw std::runtime_error("failed to write cache file: " + path.string());
    }

    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
}

[[nodiscard]] bool compressionSettingsEqual(const CacheCompressionSettings& left,
                                            const CacheCompressionSettings& right)
{
    return left.algorithm == right.algorithm && left.level == right.level;
}

[[nodiscard]] CacheCompressionSettings parseCompressionHeader(std::string_view header)
{
    CacheCompressionSettings settings;
    std::size_t offset = 0;
    while (offset < header.size())
    {
        const auto LineEnd = header.find('\n', offset);
        const std::string_view Line = header.substr(
            offset, LineEnd == std::string_view::npos ? std::string_view::npos : LineEnd - offset);
        const auto Equals = Line.find('=');
        if (Equals != std::string_view::npos)
        {
            const std::string_view Key = Line.substr(0, Equals);
            const std::string_view Value = Line.substr(Equals + 1);
            if (Key == "algorithm")
            {
                settings.algorithm = parseCacheCompressionAlgorithm(std::string(Value));
            }
            else if (Key == "level")
            {
                int level = DefaultCacheCompressionLevel;
                const auto Result = std::from_chars(Value.begin(), Value.end(), level);
                if (Result.ec == std::errc {})
                {
                    settings.level = level;
                }
            }
        }

        if (LineEnd == std::string_view::npos)
        {
            break;
        }

        offset = LineEnd + 1;
    }

    return normalizeCacheCompressionSettings(settings);
}

[[nodiscard]] std::string_view compressedPayload(std::string_view payload)
{
    const auto PayloadStart = payload.find(CacheSeparator);
    if (PayloadStart == std::string_view::npos)
    {
        throw std::runtime_error("invalid compressed cache payload");
    }

    return payload.substr(PayloadStart + CacheSeparator.size());
}

[[nodiscard]] CacheCompressionSettings compressionSettingsFromPayload(std::string_view payload)
{
    if (!payload.starts_with(CacheMagic))
    {
        throw std::runtime_error("invalid compressed cache envelope");
    }

    const auto HeaderEnd = payload.find(CacheSeparator);
    if (HeaderEnd == std::string_view::npos)
    {
        throw std::runtime_error("invalid compressed cache payload");
    }

    const std::string_view Header =
        payload.substr(CacheMagic.size(), HeaderEnd - CacheMagic.size());
    return parseCompressionHeader(Header);
}

[[nodiscard]] std::string decompressPayload(std::string_view payload)
{
    const auto Settings = compressionSettingsFromPayload(payload);
    const auto Compressor = makeCacheCompressor(Settings);
    return Compressor->decompress(compressedPayload(payload));
}

void writeCompressionMeta(const std::filesystem::path& metaPath,
                          const CacheCompressionSettings& settings)
{
    std::ostringstream stream;
    stream << "algorithm=" << toString(settings.algorithm) << '\n';
    stream << "level=" << settings.level << '\n';
    writeBinaryFile(metaPath, stream.str());
}

[[nodiscard]] CacheCompressionSettings readCompressionMeta(const std::filesystem::path& metaPath)
{
    return parseCompressionHeader(readBinaryFile(metaPath));
}

[[nodiscard]] bool migrateCacheFileCompression(const std::filesystem::path& path,
                                               const CacheOptions& options)
{
    const std::string Payload = readBinaryFile(path);
    if (!Payload.starts_with(CacheMagic))
    {
        return false;
    }

    const auto Stored = compressionSettingsFromPayload(Payload);
    const auto Target = normalizeCacheCompressionSettings(options.compress);
    if (compressionSettingsEqual(Stored, Target))
    {
        return false;
    }

    const std::string Content = decompressPayload(Payload);
    writeCacheFile(path, Content, options);
    return true;
}

}  // namespace

void prepareCacheFileForWrite(const std::filesystem::path& path, bool protect)
{
    if (!protect || !std::filesystem::exists(path))
    {
        return;
    }

    std::error_code errorCode;
    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::add,
                                 errorCode);
}

void applyCacheFileProtection(const std::filesystem::path& path, bool protect)
{
    if (!protect || !std::filesystem::exists(path))
    {
        return;
    }

    const auto ReadOnly = std::filesystem::perms::owner_read | std::filesystem::perms::group_read |
                          std::filesystem::perms::others_read;
    std::error_code errorCode;
    std::filesystem::permissions(path, ReadOnly, std::filesystem::perm_options::replace, errorCode);
}

void writeCacheFile(const std::filesystem::path& path,
                    std::string content,
                    const CacheOptions& options)
{
    prepareCacheFileForWrite(path, options.protect);

    const auto Compressor = makeCacheCompressor(options.compress);
    std::string payload;
    if (options.compress.algorithm == CacheCompressionAlgorithm::None)
    {
        payload = std::move(content);
    }
    else
    {
        payload.reserve(CacheMagic.size() + content.size());
        payload.append(CacheMagic);
        payload.append("algorithm=");
        payload.append(toString(options.compress.algorithm));
        payload.push_back('\n');
        payload.append("level=");
        payload.append(std::to_string(options.compress.level));
        payload.append(CacheSeparator);
        payload.append(Compressor->compress(content));
    }

    writeBinaryFile(path, payload);
    applyCacheFileProtection(path, options.protect);
}

std::string readCacheFile(const std::filesystem::path& path, const CacheOptions& options)
{
    (void)options;
    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error("cache file does not exist: " + path.string());
    }

    std::string payload = readBinaryFile(path);
    if (!payload.starts_with(CacheMagic))
    {
        return payload;
    }

    return decompressPayload(payload);
}

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

    if (std::filesystem::exists(MetaPath, errorCode))
    {
        const auto Stored = readCompressionMeta(MetaPath);
        if (compressionSettingsEqual(Stored, Target))
        {
            return 0;
        }
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
