#include "beez/core/cache_compress.hpp"

#include "beez/core/cache_options.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
#include <zlib.h>

namespace beez::core
{

namespace
{

constexpr std::string_view RleMagic = "BZRLE1\n";
constexpr std::size_t RleMaxRunLength = 255U;
constexpr int GzipWindowBits = 15 + 16;
constexpr int ZlibWindowBits = 15;
constexpr int DeflateWindowBits = -15;
constexpr std::size_t MinCompressionBufferSize = 64U;

class IdentityCompressor final : public ICacheCompressor
{
  public:
    [[nodiscard]] std::string compress(std::string_view data) const override
    {
        return std::string(data);
    }

    [[nodiscard]] std::string decompress(std::string_view data) const override
    {
        return std::string(data);
    }
};

class RleCompressor final : public ICacheCompressor
{
  public:
    [[nodiscard]] std::string compress(std::string_view data) const override
    {
        std::string encoded;
        encoded.reserve(RleMagic.size() + data.size());
        encoded.append(RleMagic);

        std::size_t index = 0;
        while (index < data.size())
        {
            const auto Byte = static_cast<unsigned char>(data.at(index));
            std::size_t runLength = 1;
            while (index + runLength < data.size() &&
                   static_cast<unsigned char>(data.at(index + runLength)) == Byte &&
                   runLength < RleMaxRunLength)
            {
                ++runLength;
            }

            encoded.push_back(static_cast<char>(runLength));
            encoded.push_back(static_cast<char>(Byte));
            index += runLength;
        }

        return encoded;
    }

    [[nodiscard]] std::string decompress(std::string_view data) const override
    {
        if (!data.starts_with(RleMagic))
        {
            throw std::runtime_error("invalid RLE cache payload");
        }

        std::string decoded;
        for (std::size_t index = RleMagic.size(); index + 1 < data.size(); index += 2)
        {
            const auto RunLength = static_cast<unsigned char>(data.at(index));
            const char Byte = data.at(index + 1);
            decoded.append(RunLength, Byte);
        }

        return decoded;
    }
};

class ZlibCompressor final : public ICacheCompressor
{
  public:
    explicit ZlibCompressor(CacheCompressionSettings settings) : settings_(settings) {}

    [[nodiscard]] std::string compress(std::string_view data) const override
    {
        return compressWithWindowBits(data, windowBitsForAlgorithm(settings_.algorithm));
    }

    [[nodiscard]] std::string decompress(std::string_view data) const override
    {
        return decompressWithWindowBits(data, windowBitsForAlgorithm(settings_.algorithm));
    }

  private:
    [[nodiscard]] static int windowBitsForAlgorithm(CacheCompressionAlgorithm algorithm)
    {
        switch (algorithm)
        {
        case CacheCompressionAlgorithm::Gzip:
            return GzipWindowBits;
        case CacheCompressionAlgorithm::Zlib:
            return ZlibWindowBits;
        case CacheCompressionAlgorithm::Deflate:
            return DeflateWindowBits;
        case CacheCompressionAlgorithm::None:
        case CacheCompressionAlgorithm::Rle:
            break;
        }
        return ZlibWindowBits;
    }

    [[nodiscard]] std::string compressWithWindowBits(std::string_view data, int windowBits) const
    {
        z_stream stream {};
        if (deflateInit2(&stream, settings_.level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY) !=
            Z_OK)
        {
            throw std::runtime_error("cache compression init failed");
        }

        std::vector<unsigned char> buffer(std::max<std::size_t>(
            compressBound(static_cast<uLong>(data.size())), MinCompressionBufferSize));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
        stream.avail_in = static_cast<uInt>(data.size());

        int result = Z_OK;
        while (result == Z_OK)
        {
            stream.next_out = buffer.data() + stream.total_out;
            stream.avail_out = static_cast<uInt>(buffer.size() - stream.total_out);
            result = deflate(&stream, Z_FINISH);
            if (result == Z_OK)
            {
                buffer.resize(buffer.size() * 2U);
            }
        }

        deflateEnd(&stream);
        if (result != Z_STREAM_END)
        {
            throw std::runtime_error("cache compression failed");
        }

        return {reinterpret_cast<const char*>(buffer.data()), stream.total_out};
    }

    [[nodiscard]] static std::string decompressWithWindowBits(std::string_view data, int windowBits)
    {
        z_stream stream {};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
        stream.avail_in = static_cast<uInt>(data.size());

        if (inflateInit2(&stream, windowBits) != Z_OK)
        {
            throw std::runtime_error("cache decompression init failed");
        }

        std::string output;
        output.reserve(std::max(data.size() * 4U, MinCompressionBufferSize));
        std::vector<unsigned char> buffer(
            std::max<std::size_t>(data.size(), MinCompressionBufferSize));

        while (true)
        {
            stream.next_out = buffer.data();
            stream.avail_out = static_cast<uInt>(buffer.size());
            const int Result = inflate(&stream, Z_NO_FLUSH);
            output.append(reinterpret_cast<const char*>(buffer.data()),
                          buffer.size() - stream.avail_out);

            if (Result == Z_STREAM_END)
            {
                break;
            }

            if (Result != Z_OK)
            {
                inflateEnd(&stream);
                throw std::runtime_error("cache decompression failed");
            }
        }

        inflateEnd(&stream);
        return output;
    }

    CacheCompressionSettings settings_;
};

}  // namespace

std::unique_ptr<ICacheCompressor> makeCacheCompressor(const CacheCompressionSettings& settings)
{
    const auto Normalized = normalizeCacheCompressionSettings(settings);
    switch (Normalized.algorithm)
    {
    case CacheCompressionAlgorithm::None:
        return std::make_unique<IdentityCompressor>();
    case CacheCompressionAlgorithm::Rle:
        return std::make_unique<RleCompressor>();
    case CacheCompressionAlgorithm::Gzip:
    case CacheCompressionAlgorithm::Zlib:
    case CacheCompressionAlgorithm::Deflate:
        return std::make_unique<ZlibCompressor>(Normalized);
    }
    return std::make_unique<IdentityCompressor>();
}

}  // namespace beez::core
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
