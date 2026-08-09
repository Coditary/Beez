#include "beez/core/cache/compress.hpp"

#include "beez/core/config/cache_options.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
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
constexpr std::string_view DeltaMagic = "BZDELTA1\n";
constexpr std::string_view VByteMagic = "BZVBYTE1\n";
constexpr std::size_t RleMaxRunLength = 255U;
constexpr std::uint32_t MaxVarintByteValue = 255U;
constexpr int MaxVarintShift = 35;
constexpr std::uint32_t VarintContinuationBit = 0x80U;
constexpr std::uint32_t VarintPayloadMask = 0x7FU;
constexpr unsigned VarintShiftStep = 7U;
constexpr int GzipWindowBits = 15 + 16;
constexpr int ZlibWindowBits = 15;
constexpr int DeflateWindowBits = -15;
constexpr std::size_t MinCompressionBufferSize = 64U;
constexpr std::size_t MinZlibStreamOverhead = 18U;

[[nodiscard]] std::size_t varintEncodedSize(std::uint32_t value)
{
    std::size_t size = 0;
    while (value >= VarintContinuationBit)
    {
        ++size;
        value >>= VarintShiftStep;
    }

    return size + 1;
}

[[nodiscard]] std::size_t estimateRleBodySize(std::string_view data)
{
    std::size_t size = RleMagic.size();
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

        size += 2;
        index += runLength;
    }

    return size;
}

[[nodiscard]] std::size_t estimateDeltaBodySize(std::string_view data)
{
    return DeltaMagic.size() + data.size();
}

[[nodiscard]] std::size_t estimateVByteBodySize(std::string_view data)
{
    std::size_t size = VByteMagic.size();
    size += varintEncodedSize(static_cast<std::uint32_t>(data.size()));
    size +=
        std::accumulate(data.begin(),
                        data.end(),
                        std::size_t {0},
                        [](const std::size_t Total, const char Byte)
                        { return Total + varintEncodedSize(static_cast<unsigned char>(Byte)); });
    return size;
}

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

void appendVarint(std::uint32_t value, std::string& output)
{
    while (value >= VarintContinuationBit)
    {
        output.push_back(static_cast<char>((value & VarintPayloadMask) | VarintContinuationBit));
        value >>= VarintShiftStep;
    }

    output.push_back(static_cast<char>(value));
}

[[nodiscard]] std::uint32_t readVarint(const std::string_view& data, std::size_t& offset)
{
    std::uint32_t value = 0;
    int shift = 0;
    while (offset < data.size())
    {
        const auto Byte = static_cast<unsigned char>(data.at(offset++));
        value |= static_cast<std::uint32_t>(Byte & VarintPayloadMask) << shift;
        if ((Byte & VarintContinuationBit) == 0U)
        {
            return value;
        }

        shift += static_cast<int>(VarintShiftStep);
        if (shift > MaxVarintShift)
        {
            throw std::runtime_error("invalid variable-byte cache payload");
        }
    }

    throw std::runtime_error("invalid variable-byte cache payload");
}

class DeltaCompressor final : public ICacheCompressor
{
  public:
    [[nodiscard]] std::string compress(std::string_view data) const override
    {
        std::string encoded;
        encoded.reserve(DeltaMagic.size() + data.size());
        encoded.append(DeltaMagic);
        if (data.empty())
        {
            return encoded;
        }

        encoded.push_back(data.front());
        for (std::size_t index = 1; index < data.size(); ++index)
        {
            const auto Previous = static_cast<unsigned char>(data.at(index - 1));
            const auto Current = static_cast<unsigned char>(data.at(index));
            const auto Delta =
                static_cast<std::int8_t>(static_cast<int>(Current) - static_cast<int>(Previous));
            encoded.push_back(static_cast<char>(static_cast<unsigned char>(Delta)));
        }

        return encoded;
    }

    [[nodiscard]] std::string decompress(std::string_view data) const override
    {
        if (!data.starts_with(DeltaMagic))
        {
            throw std::runtime_error("invalid delta cache payload");
        }

        if (data.size() == DeltaMagic.size())
        {
            return {};
        }

        std::string decoded;
        decoded.reserve(data.size() - DeltaMagic.size());
        decoded.push_back(data.at(DeltaMagic.size()));
        for (std::size_t index = DeltaMagic.size() + 1; index < data.size(); ++index)
        {
            const auto Previous = static_cast<unsigned char>(decoded.back());
            const auto Delta = static_cast<std::int8_t>(static_cast<unsigned char>(data.at(index)));
            decoded.push_back(
                static_cast<char>(static_cast<unsigned char>(Previous + static_cast<int>(Delta))));
        }

        return decoded;
    }
};

class VByteCompressor final : public ICacheCompressor
{
  public:
    [[nodiscard]] std::string compress(std::string_view data) const override
    {
        std::string encoded;
        encoded.reserve(VByteMagic.size() + data.size());
        encoded.append(VByteMagic);
        appendVarint(static_cast<std::uint32_t>(data.size()), encoded);
        for (const unsigned char Byte : data)
        {
            appendVarint(Byte, encoded);
        }

        return encoded;
    }

    [[nodiscard]] std::string decompress(std::string_view data) const override
    {
        if (!data.starts_with(VByteMagic))
        {
            throw std::runtime_error("invalid variable-byte cache payload");
        }

        std::size_t offset = VByteMagic.size();
        const std::uint32_t Length = readVarint(data, offset);
        std::string decoded;
        decoded.reserve(Length);
        for (std::uint32_t index = 0; index < Length; ++index)
        {
            const std::uint32_t Byte = readVarint(data, offset);
            if (Byte > MaxVarintByteValue)
            {
                throw std::runtime_error("invalid variable-byte cache payload");
            }

            decoded.push_back(static_cast<char>(Byte));
        }

        if (offset != data.size())
        {
            throw std::runtime_error("invalid variable-byte cache payload");
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
        case CacheCompressionAlgorithm::Delta:
        case CacheCompressionAlgorithm::VByte:
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

[[nodiscard]] std::optional<std::size_t> estimateBodySize(CacheCompressionAlgorithm algorithm,
                                                          std::string_view data)
{
    switch (algorithm)
    {
    case CacheCompressionAlgorithm::None:
        return data.size();
    case CacheCompressionAlgorithm::Rle:
        return estimateRleBodySize(data);
    case CacheCompressionAlgorithm::Delta:
        return estimateDeltaBodySize(data);
    case CacheCompressionAlgorithm::VByte:
        return estimateVByteBodySize(data);
    case CacheCompressionAlgorithm::Gzip:
    case CacheCompressionAlgorithm::Zlib:
    case CacheCompressionAlgorithm::Deflate:
        return std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] bool zlibMightHelp(std::string_view data, std::size_t envelopeHeaderSize)
{
    if (data.empty())
    {
        return false;
    }

    return data.size() > envelopeHeaderSize + MinZlibStreamOverhead;
}

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
    case CacheCompressionAlgorithm::Delta:
        return std::make_unique<DeltaCompressor>();
    case CacheCompressionAlgorithm::VByte:
        return std::make_unique<VByteCompressor>();
    case CacheCompressionAlgorithm::Gzip:
    case CacheCompressionAlgorithm::Zlib:
    case CacheCompressionAlgorithm::Deflate:
        return std::make_unique<ZlibCompressor>(Normalized);
    }
    return std::make_unique<IdentityCompressor>();
}

std::optional<std::size_t> estimateCacheCompressedBodySize(CacheCompressionAlgorithm algorithm,
                                                           std::string_view data)
{
    return estimateBodySize(algorithm, data);
}

bool zlibCompressionMightHelp(std::string_view data, std::size_t envelopeHeaderSize)
{
    return zlibMightHelp(data, envelopeHeaderSize);
}

}  // namespace beez::core
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
