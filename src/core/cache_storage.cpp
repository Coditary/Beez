#include "beez/core/cache_storage.hpp"

#include "beez/core/cache_compress.hpp"
#include "beez/core/cache_options.hpp"
#include "beez/core/cache_write_coordinator.hpp"

#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace beez::core
{

namespace
{

constexpr std::string_view CacheSeparator = "\n---\n";
constexpr std::string_view CompressionMetaFileName = "beez-compress.meta";

struct CacheEnvelopeParts
{
    std::string_view header;
    std::string_view body;
};

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

[[nodiscard]] std::string buildCachePayload(const std::string& content,
                                            const CacheCompressionSettings& settings)
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

void writeCompressionMeta(const std::filesystem::path& metaPath,
                          const CacheCompressionSettings& settings)
{
    std::ostringstream stream;
    stream << "algorithm=" << toString(settings.algorithm) << '\n';
    stream << "level=" << settings.level << '\n';
    stream << "mode=" << toString(settings.mode) << '\n';
    writeBinaryFile(metaPath, stream.str());
}

[[nodiscard]] bool migrateCacheFileCompression(const std::filesystem::path& path,
                                               const CacheOptions& options)
{
    const std::string CurrentOnDisk = readBinaryFile(path);
    const std::string Content = readCacheFile(path, options);
    const std::string Desired =
        buildCachePayload(Content, normalizeCacheCompressionSettings(options.compress));
    if (CurrentOnDisk == Desired)
    {
        return false;
    }

    prepareCacheFileForWrite(path, options.protect);
    writeBinaryFile(path, Desired);
    applyCacheFileProtection(path, options.protect);
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
                    const std::string& content,
                    const CacheOptions& options)
{
    if (options.writeCoordinator != nullptr && options.writeCoordinator->buffersWrites())
    {
        options.writeCoordinator->submit(path, content, options);
        return;
    }

    prepareCacheFileForWrite(path, options.protect);
    writeBinaryFile(path, buildCachePayload(content, options.compress));
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
