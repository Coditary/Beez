#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

#include "beez/core/cache/fingerprint/content_hash.hpp"
#include "beez/core/config/cache/cache_options.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace beez::plugin::lua::crypto_detail
{

namespace
{

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

[[nodiscard]] std::string bytesToHex(const std::uint8_t* data, std::size_t size)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < size; ++index)
    {
        stream << std::setw(2) << static_cast<int>(data[index]);
    }

    return stream.str();
}

[[nodiscard]] std::string readFileBytes(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        throw std::runtime_error("beez.crypto.hash_file: file does not exist: " + path.string());
    }

    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

[[nodiscard]] bool isFingerprintHash(std::string_view algorithm)
{
    const std::string Normalized = toLower(std::string(algorithm));
    const auto Names = core::contentHashAlgorithmNames();
    return std::any_of(Names.begin(), Names.end(), [&](const char* Name) { return Normalized == Name; });
}

[[nodiscard]] std::string hashFingerprint(std::string_view data, std::string_view algorithm)
{
    core::ContentHashSettings settings;
    settings.algorithm = core::parseContentHashAlgorithm(std::string(algorithm));
    const auto Hasher = core::makeContentHasher(settings);
    return Hasher->hashBytes(data);
}

// --- MD5 ---------------------------------------------------------------------

struct Md5Context
{
    std::uint32_t state[4] {};
    std::uint32_t count[2] {};
    std::uint8_t buffer[64] {};
};

constexpr std::uint32_t Md5Shift[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                      5, 9,  14, 19, 5, 9,  14, 19, 5, 9,  14, 19, 5, 9,  14, 19,
                                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

constexpr std::uint32_t Md5Table[] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

[[nodiscard]] std::uint32_t leftRotate(std::uint32_t value, std::uint32_t amount)
{
    return (value << amount) | (value >> (32U - amount));
}

void md5Transform(std::uint32_t state[4], const std::uint8_t block[64])
{
    std::uint32_t words[16];
    for (int index = 0; index < 16; ++index)
    {
        words[index] = static_cast<std::uint32_t>(block[index * 4]) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 1]) << 8U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 2]) << 16U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 3]) << 24U);
    }

    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];

    for (std::uint32_t index = 0; index < 64; ++index)
    {
        std::uint32_t function = 0;
        std::uint32_t group = 0;
        if (index < 16)
        {
            function = (b & c) | ((~b) & d);
            group = index;
        }
        else if (index < 32)
        {
            function = (d & b) | ((~d) & c);
            group = (5U * index) + 1U;
        }
        else if (index < 48)
        {
            function = b ^ c ^ d;
            group = (3U * index) + 5U;
        }
        else
        {
            function = c ^ (b | (~d));
            group = (7U * index);
        }

        const std::uint32_t temporary = d;
        d = c;
        c = b;
        b = b + leftRotate(a + function + Md5Table[index] + words[group % 16], Md5Shift[index]);
        a = temporary;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void md5Init(Md5Context& context)
{
    context.count[0] = 0;
    context.count[1] = 0;
    context.state[0] = 0x67452301U;
    context.state[1] = 0xefcdab89U;
    context.state[2] = 0x98badcfeU;
    context.state[3] = 0x10325476U;
}

void md5Update(Md5Context& context, const std::uint8_t* input, std::size_t length)
{
    std::size_t index = (context.count[0] >> 3U) & 0x3FU;
    context.count[0] += static_cast<std::uint32_t>(length << 3U);
    if (context.count[0] < static_cast<std::uint32_t>(length << 3U))
    {
        ++context.count[1];
    }

    context.count[1] += static_cast<std::uint32_t>(length >> 29U);

    std::size_t partLength = 64U - index;
    std::size_t offset = 0;
    if (length >= partLength)
    {
        std::copy_n(input, partLength, context.buffer + index);
        md5Transform(context.state, context.buffer);

        for (offset = partLength; (offset + 63U) < length; offset += 64U)
        {
            md5Transform(context.state, input + offset);
        }

        index = 0;
    }

    std::copy_n(input + offset, length - offset, context.buffer + index);
}

void md5Final(Md5Context& context, std::uint8_t digest[16])
{
    const std::uint8_t padding[64] = {0x80};
    std::uint8_t bits[8];
    for (int index = 0; index < 8; ++index)
    {
        bits[index] =
            static_cast<std::uint8_t>((context.count[index >> 2] >> ((index % 4) * 8U)) & 0xFFU);
    }

    const std::size_t Index = (context.count[0] >> 3U) & 0x3FU;
    const std::size_t PadLength = (Index < 56U) ? (56U - Index) : (120U - Index);
    md5Update(context, padding, PadLength);
    md5Update(context, bits, 8);

    for (int index = 0; index < 4; ++index)
    {
        digest[index * 4] = static_cast<std::uint8_t>(context.state[index] & 0xFFU);
        digest[(index * 4) + 1] = static_cast<std::uint8_t>((context.state[index] >> 8U) & 0xFFU);
        digest[(index * 4) + 2] = static_cast<std::uint8_t>((context.state[index] >> 16U) & 0xFFU);
        digest[(index * 4) + 3] = static_cast<std::uint8_t>((context.state[index] >> 24U) & 0xFFU);
    }
}

[[nodiscard]] std::array<std::uint8_t, 16> md5Digest(std::string_view data)
{
    Md5Context context;
    md5Init(context);
    md5Update(context, reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
    std::array<std::uint8_t, 16> digest {};
    md5Final(context, digest.data());
    return digest;
}

// --- SHA-1 -------------------------------------------------------------------

struct Sha1Context
{
    std::uint32_t state[5] {};
    std::uint32_t count[2] {};
    std::uint8_t buffer[64] {};
};

void sha1Transform(std::uint32_t state[5], const std::uint8_t block[64])
{
    std::uint32_t words[80];
    for (int index = 0; index < 16; ++index)
    {
        words[index] = (static_cast<std::uint32_t>(block[index * 4]) << 24U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 1]) << 16U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 2]) << 8U) |
                       static_cast<std::uint32_t>(block[(index * 4) + 3]);
    }

    for (int index = 16; index < 80; ++index)
    {
        words[index] = leftRotate(
            words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16], 1U);
    }

    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t e = state[4];

    for (int index = 0; index < 80; ++index)
    {
        std::uint32_t function = 0;
        std::uint32_t constant = 0;
        if (index < 20)
        {
            function = (b & c) | ((~b) & d);
            constant = 0x5a827999U;
        }
        else if (index < 40)
        {
            function = b ^ c ^ d;
            constant = 0x6ed9eba1U;
        }
        else if (index < 60)
        {
            function = (b & c) | (b & d) | (c & d);
            constant = 0x8f1bbcdcU;
        }
        else
        {
            function = b ^ c ^ d;
            constant = 0xca62c1d6U;
        }

        const std::uint32_t temporary =
            leftRotate(a, 5U) + function + e + constant + words[static_cast<std::size_t>(index)];
        e = d;
        d = c;
        c = leftRotate(b, 30U);
        b = a;
        a = temporary;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void sha1Init(Sha1Context& context)
{
    context.count[0] = 0;
    context.count[1] = 0;
    context.state[0] = 0x67452301U;
    context.state[1] = 0xefcdab89U;
    context.state[2] = 0x98badcfeU;
    context.state[3] = 0x10325476U;
    context.state[4] = 0xc3d2e1f0U;
}

void sha1Update(Sha1Context& context, const std::uint8_t* input, std::size_t length)
{
    std::size_t index = (context.count[0] >> 3U) & 0x3FU;
    context.count[0] += static_cast<std::uint32_t>(length << 3U);
    if (context.count[0] < static_cast<std::uint32_t>(length << 3U))
    {
        ++context.count[1];
    }

    context.count[1] += static_cast<std::uint32_t>(length >> 29U);

    std::size_t partLength = 64U - index;
    std::size_t offset = 0;
    if (length >= partLength)
    {
        std::copy_n(input, partLength, context.buffer + index);
        sha1Transform(context.state, context.buffer);

        for (offset = partLength; (offset + 63U) < length; offset += 64U)
        {
            sha1Transform(context.state, input + offset);
        }

        index = 0;
    }

    std::copy_n(input + offset, length - offset, context.buffer + index);
}

void sha1Final(Sha1Context& context, std::uint8_t digest[20])
{
    const std::uint8_t padding[64] = {0x80};
    std::uint8_t bits[8];
    for (int index = 0; index < 8; ++index)
    {
        bits[index] =
            static_cast<std::uint8_t>((context.count[index >> 2] >> ((index % 4) * 8U)) & 0xFFU);
    }

    const std::size_t Index = (context.count[0] >> 3U) & 0x3FU;
    const std::size_t PadLength = (Index < 56U) ? (56U - Index) : (120U - Index);
    sha1Update(context, padding, PadLength);
    sha1Update(context, bits, 8);

    for (int index = 0; index < 5; ++index)
    {
        digest[index * 4] = static_cast<std::uint8_t>((context.state[index] >> 24U) & 0xFFU);
        digest[(index * 4) + 1] = static_cast<std::uint8_t>((context.state[index] >> 16U) & 0xFFU);
        digest[(index * 4) + 2] = static_cast<std::uint8_t>((context.state[index] >> 8U) & 0xFFU);
        digest[(index * 4) + 3] = static_cast<std::uint8_t>(context.state[index] & 0xFFU);
    }
}

[[nodiscard]] std::array<std::uint8_t, 20> sha1Digest(std::string_view data)
{
    Sha1Context context;
    sha1Init(context);
    sha1Update(context, reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
    std::array<std::uint8_t, 20> digest {};
    sha1Final(context, digest.data());
    return digest;
}

// --- SHA-256 -----------------------------------------------------------------

constexpr std::uint32_t Sha256Constants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

[[nodiscard]] std::uint32_t rightRotate(std::uint32_t value, std::uint32_t amount)
{
    return (value >> amount) | (value << (32U - amount));
}

struct Sha256Context
{
    std::uint8_t data[64] {};
    std::uint32_t datalen = 0;
    std::uint64_t bitlen = 0;
    std::uint32_t state[8] {};
};

void sha256Transform(Sha256Context& context, const std::uint8_t block[64])
{
    std::uint32_t words[64];
    for (int index = 0; index < 16; ++index)
    {
        words[index] = (static_cast<std::uint32_t>(block[index * 4]) << 24U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 1]) << 16U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 2]) << 8U) |
                       static_cast<std::uint32_t>(block[(index * 4) + 3]);
    }

    for (int index = 16; index < 64; ++index)
    {
        const std::uint32_t Sigma0 = rightRotate(words[index - 15], 7U) ^
                                     rightRotate(words[index - 15], 18U) ^
                                     (words[index - 15] >> 3U);
        const std::uint32_t Sigma1 = rightRotate(words[index - 2], 17U) ^
                                     rightRotate(words[index - 2], 19U) ^ (words[index - 2] >> 10U);
        words[index] = words[index - 16] + Sigma0 + words[index - 7] + Sigma1;
    }

    std::uint32_t a = context.state[0];
    std::uint32_t b = context.state[1];
    std::uint32_t c = context.state[2];
    std::uint32_t d = context.state[3];
    std::uint32_t e = context.state[4];
    std::uint32_t f = context.state[5];
    std::uint32_t g = context.state[6];
    std::uint32_t h = context.state[7];

    for (int index = 0; index < 64; ++index)
    {
        const std::uint32_t Sigma1 = rightRotate(e, 6U) ^ rightRotate(e, 11U) ^ rightRotate(e, 25U);
        const std::uint32_t Ch = (e & f) ^ ((~e) & g);
        const std::uint32_t temporary1 = h + Sigma1 + Ch + Sha256Constants[index] + words[index];
        const std::uint32_t Sigma0 = rightRotate(a, 2U) ^ rightRotate(a, 13U) ^ rightRotate(a, 22U);
        const std::uint32_t Maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temporary2 = Sigma0 + Maj;

        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }

    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

void sha256Init(Sha256Context& context)
{
    context.datalen = 0;
    context.bitlen = 0;
    context.state[0] = 0x6a09e667U;
    context.state[1] = 0xbb67ae85U;
    context.state[2] = 0x3c6ef372U;
    context.state[3] = 0xa54ff53aU;
    context.state[4] = 0x510e527fU;
    context.state[5] = 0x9b05688cU;
    context.state[6] = 0x1f83d9abU;
    context.state[7] = 0x5be0cd19U;
}

void sha256Update(Sha256Context& context, const std::uint8_t* input, std::size_t length)
{
    for (std::size_t index = 0; index < length; ++index)
    {
        context.data[context.datalen] = input[index];
        ++context.datalen;
        if (context.datalen == 64U)
        {
            sha256Transform(context, context.data);
            context.bitlen += 512U;
            context.datalen = 0;
        }
    }
}

void sha256Final(Sha256Context& context, std::uint8_t digest[32])
{
    std::size_t index = context.datalen;

    if (context.datalen < 56U)
    {
        context.data[index++] = 0x80U;
        while (index < 56U)
        {
            context.data[index++] = 0U;
        }
    }
    else
    {
        context.data[index++] = 0x80U;
        while (index < 64U)
        {
            context.data[index++] = 0U;
        }

        sha256Transform(context, context.data);
        std::fill_n(context.data, 56, 0U);
    }

    context.bitlen += context.datalen * 8U;
    context.data[56] = static_cast<std::uint8_t>((context.bitlen >> 56U) & 0xFFU);
    context.data[57] = static_cast<std::uint8_t>((context.bitlen >> 48U) & 0xFFU);
    context.data[58] = static_cast<std::uint8_t>((context.bitlen >> 40U) & 0xFFU);
    context.data[59] = static_cast<std::uint8_t>((context.bitlen >> 32U) & 0xFFU);
    context.data[60] = static_cast<std::uint8_t>((context.bitlen >> 24U) & 0xFFU);
    context.data[61] = static_cast<std::uint8_t>((context.bitlen >> 16U) & 0xFFU);
    context.data[62] = static_cast<std::uint8_t>((context.bitlen >> 8U) & 0xFFU);
    context.data[63] = static_cast<std::uint8_t>(context.bitlen & 0xFFU);

    sha256Transform(context, context.data);

    for (int word = 0; word < 8; ++word)
    {
        digest[word * 4] = static_cast<std::uint8_t>((context.state[word] >> 24U) & 0xFFU);
        digest[(word * 4) + 1] = static_cast<std::uint8_t>((context.state[word] >> 16U) & 0xFFU);
        digest[(word * 4) + 2] = static_cast<std::uint8_t>((context.state[word] >> 8U) & 0xFFU);
        digest[(word * 4) + 3] = static_cast<std::uint8_t>(context.state[word] & 0xFFU);
    }
}

[[nodiscard]] std::array<std::uint8_t, 32> sha256Digest(std::string_view data)
{
    Sha256Context context;
    sha256Init(context);
    sha256Update(context, reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
    std::array<std::uint8_t, 32> digest {};
    sha256Final(context, digest.data());
    return digest;
}

// --- SHA-512 -----------------------------------------------------------------

constexpr std::uint64_t Sha512Constants[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

[[nodiscard]] std::uint64_t rightRotate64(std::uint64_t value, std::uint64_t amount)
{
    return (value >> amount) | (value << (64U - amount));
}

struct Sha512Context
{
    std::uint8_t data[128] {};
    std::uint32_t datalen = 0;
    std::uint64_t bitlen[2] {};
    std::uint64_t state[8] {};
};

void sha512Transform(Sha512Context& context, const std::uint8_t block[128])
{
    std::uint64_t words[80];
    for (int index = 0; index < 16; ++index)
    {
        words[index] = (static_cast<std::uint64_t>(block[index * 8]) << 56U) |
                       (static_cast<std::uint64_t>(block[(index * 8) + 1]) << 48U) |
                       (static_cast<std::uint64_t>(block[(index * 8) + 2]) << 40U) |
                       (static_cast<std::uint64_t>(block[(index * 8) + 3]) << 32U) |
                       (static_cast<std::uint64_t>(block[(index * 8) + 4]) << 24U) |
                       (static_cast<std::uint64_t>(block[(index * 8) + 5]) << 16U) |
                       (static_cast<std::uint64_t>(block[(index * 8) + 6]) << 8U) |
                       static_cast<std::uint64_t>(block[(index * 8) + 7]);
    }

    for (int index = 16; index < 80; ++index)
    {
        const std::uint64_t Sigma0 = rightRotate64(words[index - 15], 1U) ^
                                     rightRotate64(words[index - 15], 8U) ^
                                     (words[index - 15] >> 7U);
        const std::uint64_t Sigma1 = rightRotate64(words[index - 2], 19U) ^
                                     rightRotate64(words[index - 2], 61U) ^
                                     (words[index - 2] >> 6U);
        words[index] = words[index - 16] + Sigma0 + words[index - 7] + Sigma1;
    }

    std::uint64_t a = context.state[0];
    std::uint64_t b = context.state[1];
    std::uint64_t c = context.state[2];
    std::uint64_t d = context.state[3];
    std::uint64_t e = context.state[4];
    std::uint64_t f = context.state[5];
    std::uint64_t g = context.state[6];
    std::uint64_t h = context.state[7];

    for (int index = 0; index < 80; ++index)
    {
        const std::uint64_t Sigma1 =
            rightRotate64(e, 14U) ^ rightRotate64(e, 18U) ^ rightRotate64(e, 41U);
        const std::uint64_t Ch = (e & f) ^ ((~e) & g);
        const std::uint64_t temporary1 = h + Sigma1 + Ch + Sha512Constants[index] + words[index];
        const std::uint64_t Sigma0 =
            rightRotate64(a, 28U) ^ rightRotate64(a, 34U) ^ rightRotate64(a, 39U);
        const std::uint64_t Maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint64_t temporary2 = Sigma0 + Maj;

        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }

    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

void sha512Init(Sha512Context& context)
{
    context.datalen = 0;
    context.bitlen[0] = 0;
    context.bitlen[1] = 0;
    context.state[0] = 0x6a09e667f3bcc908ULL;
    context.state[1] = 0xbb67ae8584caa73bULL;
    context.state[2] = 0x3c6ef372fe94f82bULL;
    context.state[3] = 0xa54ff53a5f1d36f1ULL;
    context.state[4] = 0x510e527fade682d1ULL;
    context.state[5] = 0x9b05688c2b3e6c1fULL;
    context.state[6] = 0x1f83d9abfb41bd6bULL;
    context.state[7] = 0x5be0cd19137e2179ULL;
}

void sha512Update(Sha512Context& context, const std::uint8_t* input, std::size_t length)
{
    for (std::size_t index = 0; index < length; ++index)
    {
        context.data[context.datalen] = input[index];
        ++context.datalen;
        if (context.datalen == 128U)
        {
            sha512Transform(context, context.data);
            context.bitlen[0] += 1024U;
            if (context.bitlen[0] < 1024U)
            {
                ++context.bitlen[1];
            }

            context.datalen = 0;
        }
    }
}

void sha512Final(Sha512Context& context, std::uint8_t digest[64])
{
    std::size_t index = context.datalen;

    if (context.datalen < 112U)
    {
        context.data[index++] = 0x80U;
        while (index < 112U)
        {
            context.data[index++] = 0U;
        }
    }
    else
    {
        context.data[index++] = 0x80U;
        while (index < 128U)
        {
            context.data[index++] = 0U;
        }

        sha512Transform(context, context.data);
        std::fill_n(context.data, 112, 0U);
    }

    const std::uint64_t LowBits = context.bitlen[0] + (context.datalen * 8U);
    if (context.bitlen[0] > LowBits)
    {
        ++context.bitlen[1];
    }

    context.bitlen[0] = LowBits;

    for (int byte = 0; byte < 8; ++byte)
    {
        context.data[120 + byte] =
            static_cast<std::uint8_t>((context.bitlen[1] >> (56 - (byte * 8))) & 0xFFU);
    }

    for (int byte = 0; byte < 8; ++byte)
    {
        context.data[112 + byte] =
            static_cast<std::uint8_t>((context.bitlen[0] >> (56 - (byte * 8))) & 0xFFU);
    }

    sha512Transform(context, context.data);

    for (int word = 0; word < 8; ++word)
    {
        for (int byte = 0; byte < 8; ++byte)
        {
            digest[(word * 8) + byte] =
                static_cast<std::uint8_t>((context.state[word] >> (56 - (byte * 8))) & 0xFFU);
        }
    }
}

[[nodiscard]] std::array<std::uint8_t, 64> sha512Digest(std::string_view data)
{
    Sha512Context context;
    sha512Init(context);
    sha512Update(context, reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
    std::array<std::uint8_t, 64> digest {};
    sha512Final(context, digest.data());
    return digest;
}

enum class CryptoDigestAlgorithm
{
    Sha256,
    Sha512,
    Sha1,
    Md5,
};

[[nodiscard]] bool parseCryptoDigestAlgorithm(std::string_view algorithm,
                                              CryptoDigestAlgorithm& out)
{
    const std::string Normalized = toLower(std::string(algorithm));
    if (Normalized == "sha256")
    {
        out = CryptoDigestAlgorithm::Sha256;
        return true;
    }
    if (Normalized == "sha512")
    {
        out = CryptoDigestAlgorithm::Sha512;
        return true;
    }
    if (Normalized == "sha1")
    {
        out = CryptoDigestAlgorithm::Sha1;
        return true;
    }
    if (Normalized == "md5")
    {
        out = CryptoDigestAlgorithm::Md5;
        return true;
    }

    return false;
}

[[nodiscard]] std::string digestToHexString(CryptoDigestAlgorithm algorithm, std::string_view data)
{
    switch (algorithm)
    {
    case CryptoDigestAlgorithm::Sha256:
    {
        const auto Digest = sha256Digest(data);
        return bytesToHex(Digest.data(), Digest.size());
    }
    case CryptoDigestAlgorithm::Sha512:
    {
        const auto Digest = sha512Digest(data);
        return bytesToHex(Digest.data(), Digest.size());
    }
    case CryptoDigestAlgorithm::Sha1:
    {
        const auto Digest = sha1Digest(data);
        return bytesToHex(Digest.data(), Digest.size());
    }
    case CryptoDigestAlgorithm::Md5:
    {
        const auto Digest = md5Digest(data);
        return bytesToHex(Digest.data(), Digest.size());
    }
    }

    return {};
}

[[nodiscard]] std::vector<std::uint8_t> digestBytes(CryptoDigestAlgorithm algorithm,
                                                    std::string_view data)
{
    switch (algorithm)
    {
    case CryptoDigestAlgorithm::Sha256:
    {
        const auto Digest = sha256Digest(data);
        return {Digest.begin(), Digest.end()};
    }
    case CryptoDigestAlgorithm::Sha512:
    {
        const auto Digest = sha512Digest(data);
        return {Digest.begin(), Digest.end()};
    }
    case CryptoDigestAlgorithm::Sha1:
    {
        const auto Digest = sha1Digest(data);
        return {Digest.begin(), Digest.end()};
    }
    case CryptoDigestAlgorithm::Md5:
    {
        const auto Digest = md5Digest(data);
        return {Digest.begin(), Digest.end()};
    }
    }

    return {};
}

[[nodiscard]] std::vector<std::uint8_t>
hmacDigestBytes(CryptoDigestAlgorithm algorithm, std::string_view key, std::string_view data)
{
    constexpr std::size_t Sha256BlockSize = 64;
    constexpr std::size_t Sha512BlockSize = 128;
    const std::size_t BlockSize =
        algorithm == CryptoDigestAlgorithm::Sha512 ? Sha512BlockSize : Sha256BlockSize;

    std::vector<std::uint8_t> keyBytes(key.begin(), key.end());
    if (keyBytes.size() > BlockSize)
    {
        keyBytes = digestBytes(algorithm, key);
    }

    keyBytes.resize(BlockSize, 0);

    std::vector<std::uint8_t> innerPad(BlockSize, 0x36);
    std::vector<std::uint8_t> outerPad(BlockSize, 0x5c);
    for (std::size_t index = 0; index < BlockSize; ++index)
    {
        innerPad[index] ^= keyBytes[index];
        outerPad[index] ^= keyBytes[index];
    }

    std::string innerMessage;
    innerMessage.reserve(innerPad.size() + data.size());
    innerMessage.append(reinterpret_cast<const char*>(innerPad.data()), innerPad.size());
    innerMessage.append(data);

    const std::vector<std::uint8_t> InnerDigest = digestBytes(algorithm, innerMessage);

    std::string outerMessage;
    outerMessage.reserve(outerPad.size() + InnerDigest.size());
    outerMessage.append(reinterpret_cast<const char*>(outerPad.data()), outerPad.size());
    outerMessage.append(reinterpret_cast<const char*>(InnerDigest.data()), InnerDigest.size());

    return digestBytes(algorithm, outerMessage);
}

[[nodiscard]] std::string encodeHex(std::string_view data)
{
    return bytesToHex(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
}

[[nodiscard]] std::string encodeBase64(std::string_view data)
{
    static constexpr char EncodingTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((data.size() + 2U) / 3U) * 4U);

    std::size_t index = 0;
    while (index + 2U < data.size())
    {
        const auto Byte0 = static_cast<unsigned char>(data[index]);
        const auto Byte1 = static_cast<unsigned char>(data[index + 1U]);
        const auto Byte2 = static_cast<unsigned char>(data[index + 2U]);
        encoded.push_back(EncodingTable[(Byte0 >> 2U) & 0x3FU]);
        encoded.push_back(EncodingTable[((Byte0 << 4U) | (Byte1 >> 4U)) & 0x3FU]);
        encoded.push_back(EncodingTable[((Byte1 << 2U) | (Byte2 >> 6U)) & 0x3FU]);
        encoded.push_back(EncodingTable[Byte2 & 0x3FU]);
        index += 3U;
    }

    if (index < data.size())
    {
        const auto Byte0 = static_cast<unsigned char>(data[index]);
        encoded.push_back(EncodingTable[(Byte0 >> 2U) & 0x3FU]);
        if ((index + 1U) < data.size())
        {
            const auto Byte1 = static_cast<unsigned char>(data[index + 1U]);
            encoded.push_back(EncodingTable[((Byte0 << 4U) | (Byte1 >> 4U)) & 0x3FU]);
            encoded.push_back(EncodingTable[(Byte1 << 2U) & 0x3FU]);
            encoded.push_back('=');
        }
        else
        {
            encoded.push_back(EncodingTable[(Byte0 << 4U) & 0x3FU]);
            encoded.push_back('=');
            encoded.push_back('=');
        }
    }

    return encoded;
}

}  // namespace

std::vector<std::string> supportedHashAlgorithms()
{
    std::vector<std::string> algorithms = {"sha256", "sha512", "sha1", "md5"};
    const auto Names = core::contentHashAlgorithmNames();
    algorithms.insert(algorithms.end(), Names.begin(), Names.end());

    return algorithms;
}

std::vector<std::string> supportedEncodingAlgorithms()
{
    return {"hex", "base64"};
}

bool isHashAlgorithm(const std::string_view name)
{
    if (isFingerprintHash(name))
    {
        return true;
    }

    CryptoDigestAlgorithm algorithm {};
    return parseCryptoDigestAlgorithm(name, algorithm);
}

bool isEncodingAlgorithm(const std::string_view name)
{
    const std::string Normalized = toLower(std::string(name));
    return Normalized == "hex" || Normalized == "base64";
}

std::string hashString(const std::string_view data, const std::string_view algorithm)
{
    if (isFingerprintHash(algorithm))
    {
        return hashFingerprint(data, algorithm);
    }

    CryptoDigestAlgorithm digestAlgorithm {};
    if (!parseCryptoDigestAlgorithm(algorithm, digestAlgorithm))
    {
        throw std::runtime_error("beez.crypto.hash_string: unknown hash algorithm: " +
                                 std::string(algorithm));
    }

    return digestToHexString(digestAlgorithm, data);
}

std::string hashFile(const std::filesystem::path& path, const std::string_view algorithm)
{
    return hashString(readFileBytes(path), algorithm);
}

std::string encodeString(const std::string_view data, const std::string_view encoding)
{
    const std::string Normalized = toLower(std::string(encoding));
    if (Normalized == "hex")
    {
        return encodeHex(data);
    }
    if (Normalized == "base64")
    {
        return encodeBase64(data);
    }

    throw std::runtime_error("beez.crypto.encode: unknown encoding algorithm: " +
                             std::string(encoding));
}

std::string encodeWithKey(const std::string_view data,
                          const std::string_view key,
                          const std::string_view hashAlgorithm)
{
    if (isFingerprintHash(hashAlgorithm))
    {
        throw std::runtime_error(
            "beez.crypto.encode: HMAC is not supported for fingerprint hash '" +
            std::string(hashAlgorithm) + "'");
    }

    CryptoDigestAlgorithm digestAlgorithm {};
    if (!parseCryptoDigestAlgorithm(hashAlgorithm, digestAlgorithm))
    {
        throw std::runtime_error("beez.crypto.encode: unknown hash algorithm: " +
                                 std::string(hashAlgorithm));
    }

    const std::vector<std::uint8_t> Digest = hmacDigestBytes(digestAlgorithm, key, data);
    return bytesToHex(Digest.data(), Digest.size());
}

}  // namespace beez::plugin::lua::crypto_detail
