#include "beez/core/cache/fingerprint/content_hash.hpp"

#include "beez/core/config/cache/cache_options.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#ifdef __linux__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace beez::core
{

namespace
{

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
constexpr std::uint64_t FnvOffsetBasis64 = 0xcbf29ce484222325ULL;
constexpr std::uint64_t FnvPrime64 = 0x100000001b3ULL;
constexpr std::uint32_t FnvOffsetBasis32 = 0x811c9dc5U;
constexpr std::uint32_t FnvPrime32 = 0x01000193U;
constexpr int HexDigestWidth64 = 16;
constexpr int HexDigestWidth32 = 8;

[[nodiscard]] std::uint64_t fnv1a64(std::string_view data, std::uint64_t seed)
{
    std::uint64_t hash = FnvOffsetBasis64 ^ seed;
    for (const unsigned char ByteCharacter : data)
    {
        hash ^= static_cast<std::uint64_t>(ByteCharacter);
        hash *= FnvPrime64;
    }
    return hash;
}

[[nodiscard]] std::uint32_t fnv1a32(std::string_view data, std::uint32_t seed)
{
    std::uint32_t hash = FnvOffsetBasis32 ^ seed;
    for (const unsigned char ByteCharacter : data)
    {
        hash ^= static_cast<std::uint32_t>(ByteCharacter);
        hash *= FnvPrime32;
    }
    return hash;
}

[[nodiscard]] std::uint32_t crc32(std::string_view data, std::uint32_t seed)
{
    std::uint32_t hash = seed ^ 0xFFFFFFFFU;
    for (const unsigned char ByteCharacter : data)
    {
        hash ^= static_cast<std::uint32_t>(ByteCharacter);
        for (int bit = 0; bit < 8; ++bit)
        {
            const std::uint32_t Mask = -(hash & 1U);
            hash = (hash >> 1U) ^ (0xEDB88320U & Mask);
        }
    }
    return hash ^ 0xFFFFFFFFU;
}

[[nodiscard]] std::uint32_t djb2(std::string_view data, std::uint32_t seed)
{
    std::uint32_t hash = 5381U ^ seed;
    for (const unsigned char ByteCharacter : data)
    {
        // cppcheck-suppress useStlAlgorithm
        hash = ((hash << 5U) + hash) + static_cast<std::uint32_t>(ByteCharacter);
    }
    return hash;
}

[[nodiscard]] std::uint32_t sdbm(std::string_view data, std::uint32_t seed)
{
    std::uint32_t hash = seed;
    for (const unsigned char ByteCharacter : data)
    {
        // cppcheck-suppress useStlAlgorithm
        hash = static_cast<std::uint32_t>(ByteCharacter) + (hash << 6U) + (hash << 16U) - hash;
    }
    return hash;
}

[[nodiscard]] std::string digestToHex(std::uint64_t digest)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(HexDigestWidth64) << digest;
    return stream.str();
}

[[nodiscard]] std::string digestToHex(std::uint32_t digest)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(HexDigestWidth32) << digest;
    return stream.str();
}

class ContentHasher final : public IContentHasher
{
  public:
    explicit ContentHasher(const ContentHashSettings& settings) : settings_(settings) {}

    [[nodiscard]] std::string hashBytes(std::string_view data) const override
    {
        switch (settings_.algorithm)
        {
        case ContentHashAlgorithm::Fnv1a64:
            return digestToHex(fnv1a64(data, settings_.seed));
        case ContentHashAlgorithm::Fnv1a32:
            return digestToHex(fnv1a32(data, settings_.seed));
        case ContentHashAlgorithm::Crc32:
            return digestToHex(crc32(data, settings_.seed));
        case ContentHashAlgorithm::Djb2:
            return digestToHex(djb2(data, settings_.seed));
        case ContentHashAlgorithm::Sdbm:
            return digestToHex(sdbm(data, settings_.seed));
        }
        return digestToHex(fnv1a64(data, settings_.seed));
    }

    [[nodiscard]] std::string hashFile(const std::filesystem::path& path) const override
    {
#ifdef __linux__
        if (settings_.useMmapForHashing)
        {
            // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)
            struct stat fileStat {};
            if (stat(path.c_str(), &fileStat) == 0 &&
                std::cmp_greater_equal(static_cast<std::size_t>(fileStat.st_size),
                                       settings_.mmapHashingMinBytes))
            {
                const int Descriptor = open(path.c_str(), O_RDONLY);
                if (Descriptor >= 0)
                {
                    void* mapped = mmap(nullptr,
                                        static_cast<std::size_t>(fileStat.st_size),
                                        PROT_READ,
                                        MAP_PRIVATE,
                                        Descriptor,
                                        0);
                    close(Descriptor);
                    if (mapped != MAP_FAILED)
                    {
                        const std::string_view View(static_cast<const char*>(mapped),
                                                    static_cast<std::size_t>(fileStat.st_size));
                        const auto Digest = hashBytes(View);
                        munmap(mapped, static_cast<std::size_t>(fileStat.st_size));
                        return Digest;
                    }
                }
            }
            // NOLINTEND(cppcoreguidelines-pro-type-vararg)
        }
#endif

        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open())
        {
            // A missing/unreadable file must not hash like an empty file.
            return hashBytes("\x01missing\x01" + path.generic_string());
        }

        return hashBytes(
            std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()));
    }

    [[nodiscard]] std::string combine(std::initializer_list<std::string_view> parts) const override
    {
        std::string combined;
        for (const auto& part : parts)
        {
            combined.append(part);
            combined.push_back('\0');
        }
        return hashBytes(combined);
    }

  private:
    ContentHashSettings settings_;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}  // namespace

std::unique_ptr<IContentHasher> makeContentHasher(const ContentHashSettings& settings)
{
    return std::make_unique<ContentHasher>(normalizeContentHashSettings(settings));
}

std::unique_ptr<IContentHasher> makeSha256Hasher()
{
    return makeContentHasher(ContentHashSettings {});
}

}  // namespace beez::core
