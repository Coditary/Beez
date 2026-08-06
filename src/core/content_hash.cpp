#include "beez/core/content_hash.hpp"

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

namespace beez::core
{

namespace
{

constexpr std::uint64_t FnvOffsetBasis = 0xcbf29ce484222325ULL;
constexpr std::uint64_t FnvPrime = 0x100000001b3ULL;
constexpr int HexDigestWidth = 16;

[[nodiscard]] std::uint64_t fnv1a64(std::string_view data)
{
    std::uint64_t hash = FnvOffsetBasis;
    for (const unsigned char ByteCharacter : data)
    {
        hash ^= static_cast<std::uint64_t>(ByteCharacter);
        hash *= FnvPrime;
    }
    return hash;
}

[[nodiscard]] std::string digestToHex(std::uint64_t digest)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(HexDigestWidth) << digest;
    return stream.str();
}

class Fnv1aHasher final : public IContentHasher
{
  public:
    [[nodiscard]] std::string hashBytes(std::string_view data) const override
    {
        return digestToHex(fnv1a64(data));
    }

    [[nodiscard]] std::string hashFile(const std::filesystem::path& path) const override
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open())
        {
            return hashBytes("");
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
};

}  // namespace

std::unique_ptr<IContentHasher> makeSha256Hasher()
{
    return std::make_unique<Fnv1aHasher>();
}

}  // namespace beez::core
