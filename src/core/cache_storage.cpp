#include "beez/core/cache_storage.hpp"

#include "beez/core/cache_compress.hpp"
#include "beez/core/cache_options.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
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
        payload.append("\n---\n");
        payload.append(Compressor->compress(content));
    }

    writeBinaryFile(path, payload);
    applyCacheFileProtection(path, options.protect);
}

std::string readCacheFile(const std::filesystem::path& path, const CacheOptions& options)
{
    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error("cache file does not exist: " + path.string());
    }

    std::string payload = readBinaryFile(path);
    if (!payload.starts_with(CacheMagic))
    {
        return payload;
    }

    const auto PayloadStart = payload.find("\n---\n");
    if (PayloadStart == std::string::npos)
    {
        throw std::runtime_error("invalid compressed cache payload: " + path.string());
    }

    const auto Compressed = payload.substr(PayloadStart + 5U);
    const auto Compressor = makeCacheCompressor(options.compress);
    return Compressor->decompress(Compressed);
}

}  // namespace beez::core
