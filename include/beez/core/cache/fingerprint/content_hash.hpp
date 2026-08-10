#pragma once

#include "beez/core/config/cache/cache_options.hpp"

#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>

namespace beez::core
{

class IContentHasher
{
  public:
    IContentHasher() = default;
    virtual ~IContentHasher() = default;

    IContentHasher(const IContentHasher&) = delete;
    IContentHasher& operator=(const IContentHasher&) = delete;
    IContentHasher(IContentHasher&&) = delete;
    IContentHasher& operator=(IContentHasher&&) = delete;

    [[nodiscard]] virtual std::string hashBytes(std::string_view data) const = 0;
    [[nodiscard]] virtual std::string hashFile(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual std::string
    combine(std::initializer_list<std::string_view> parts) const = 0;
};

[[nodiscard]] std::unique_ptr<IContentHasher>
makeContentHasher(const ContentHashSettings& settings);

[[nodiscard]] std::unique_ptr<IContentHasher> makeSha256Hasher();

}  // namespace beez::core
