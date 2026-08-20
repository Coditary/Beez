#include "beez/core/cache/storage/envelope.hpp"

#include "beez/core/cache/storage/compress.hpp"
#include "beez/core/cache/storage/write_coordinator.hpp"
#include "beez/core/config/cache/cache_options.hpp"
#include "storage_detail.hpp"

#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace beez::core
{

namespace
{

constexpr std::string_view CacheSeparator = "\n---\n";

struct CacheEnvelopeParts
{
    std::string_view header;
    std::string_view body;
};

[[nodiscard]] std::optional<CacheEnvelopeParts> trySplitCacheEnvelope(std::string_view payload)
{
    std::size_t lineStart = 0;
    while (lineStart < payload.size())
    {
        const auto LineEnd = payload.find('\n', lineStart);
        const std::string_view Line = payload.substr(
            lineStart,
            LineEnd == std::string_view::npos ? std::string_view::npos : LineEnd - lineStart);
        if (Line == "---")
        {
            const std::size_t BodyStart =
                LineEnd == std::string_view::npos ? payload.size() : LineEnd + 1;
            return CacheEnvelopeParts {
                .header = payload.substr(0, lineStart),
                .body = payload.substr(BodyStart),
            };
        }

        if (LineEnd == std::string_view::npos)
        {
            break;
        }

        lineStart = LineEnd + 1;
    }

    return std::nullopt;
}

[[nodiscard]] std::size_t cacheEnvelopeHeaderSize(const CacheCompressionSettings& settings)
{
    std::string header;
    header.append("algorithm=");
    header.append(toString(settings.algorithm));
    header.push_back('\n');
    header.append("level=");
    header.append(std::to_string(settings.level));
    header.push_back('\n');
    header.append("mode=");
    header.append(toString(settings.mode));
    header.append(CacheSeparator);
    return header.size();
}

[[nodiscard]] std::string buildCompressedEnvelope(const CacheCompressionSettings& settings,
                                                  const std::string& compressedBody)
{
    std::string payload;
    payload.reserve(cacheEnvelopeHeaderSize(settings) + compressedBody.size());
    payload.append("algorithm=");
    payload.append(toString(settings.algorithm));
    payload.push_back('\n');
    payload.append("level=");
    payload.append(std::to_string(settings.level));
    payload.push_back('\n');
    payload.append("mode=");
    payload.append(toString(settings.mode));
    payload.append(CacheSeparator);
    payload.append(compressedBody);
    return payload;
}

[[nodiscard]] std::string buildCompressedEnvelope(const std::string& content,
                                                  const CacheCompressionSettings& settings)
{
    const auto Compressor = makeCacheCompressor(settings);
    return buildCompressedEnvelope(settings, Compressor->compress(content));
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
            else if (Key == "mode")
            {
                settings.mode = parseCacheCompressionMode(std::string(Value));
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

[[nodiscard]] std::string decodeEnvelopeBody(const CacheEnvelopeParts& envelope)
{
    if (envelope.header.empty())
    {
        return std::string(envelope.body);
    }

    const auto Settings = parseCompressionHeader(envelope.header);
    if (Settings.algorithm == CacheCompressionAlgorithm::None)
    {
        return std::string(envelope.body);
    }

    const auto Compressor = makeCacheCompressor(Settings);
    return Compressor->decompress(envelope.body);
}

}  // namespace

namespace storage_detail
{

std::string readBinaryFile(const std::filesystem::path& path)
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

    // Write via a temporary file and rename atomically, so a crash mid-write or a
    // concurrent reader never observes a truncated cache file.
    static std::atomic<std::uint64_t> tempCounter {0};
    const std::filesystem::path TempPath =
        path.parent_path() /
        (path.filename().string() + ".tmp." + std::to_string(tempCounter.fetch_add(1)));
    {
        std::ofstream stream(TempPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open())
        {
            throw std::runtime_error("failed to write cache file: " + path.string());
        }

        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        stream.flush();
        if (!stream.good())
        {
            stream.close();
            std::error_code cleanupError;
            std::filesystem::remove(TempPath, cleanupError);
            throw std::runtime_error("failed to write cache file: " + path.string());
        }
    }

    std::error_code errorCode;
    std::filesystem::rename(TempPath, path, errorCode);
    if (errorCode)
    {
        // Platforms without rename-over-existing support need a remove first.
        std::error_code removeError;
        std::filesystem::remove(path, removeError);
        errorCode.clear();
        std::filesystem::rename(TempPath, path, errorCode);
        if (errorCode)
        {
            std::error_code cleanupError;
            std::filesystem::remove(TempPath, cleanupError);
            throw std::runtime_error("failed to write cache file: " + path.string() + " (" +
                                     errorCode.message() + ")");
        }
    }
}

std::string buildCachePayload(const std::string& content, const CacheCompressionSettings& settings)
{
    const auto Target = normalizeCacheCompressionSettings(settings);
    if (Target.algorithm == CacheCompressionAlgorithm::None ||
        Target.mode == CacheCompressionMode::Never)
    {
        return content;
    }

    if (Target.mode == CacheCompressionMode::Always)
    {
        return buildCompressedEnvelope(content, Target);
    }

    const std::size_t HeaderSize = cacheEnvelopeHeaderSize(Target);
    if (const auto BodySize = estimateCacheCompressedBodySize(Target.algorithm, content))
    {
        if (HeaderSize + *BodySize >= content.size())
        {
            return content;
        }

        const auto Compressor = makeCacheCompressor(Target);
        return buildCompressedEnvelope(Target, Compressor->compress(content));
    }

    if (!zlibCompressionMightHelp(content, HeaderSize))
    {
        return content;
    }

    const auto Compressor = makeCacheCompressor(Target);
    std::string envelope = buildCompressedEnvelope(Target, Compressor->compress(content));
    if (envelope.size() < content.size())
    {
        return envelope;
    }

    return content;
}

}  // namespace storage_detail

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
                    const std::string& content,
                    const CacheOptions& options)
{
    if (options.writeCoordinator != nullptr && options.writeCoordinator->buffersWrites())
    {
        options.writeCoordinator->submit(path, content, options);
        return;
    }

    prepareCacheFileForWrite(path, options.protect);
    storage_detail::writeBinaryFile(path,
                                    storage_detail::buildCachePayload(content, options.compress));
    applyCacheFileProtection(path, options.protect);
}

std::string readCacheFile(const std::filesystem::path& path, const CacheOptions& options)
{
    (void)options;
    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error("cache file does not exist: " + path.string());
    }

    std::string payload = storage_detail::readBinaryFile(path);
    if (payload.starts_with("BEEZCACHE1"))
    {
        throw std::runtime_error(
            "unsupported legacy cache envelope (BEEZCACHE1); run beez --clean-cache");
    }

    const auto Envelope = trySplitCacheEnvelope(payload);
    if (!Envelope.has_value())
    {
        return payload;
    }

    return decodeEnvelopeBody(*Envelope);
}

}  // namespace beez::core
