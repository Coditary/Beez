#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace beez::cli
{

inline constexpr std::size_t EmbeddedCompletionFileCount = 5;

// NOLINTBEGIN(misc-non-private-member-variables-in-classes) -- aggregate-like embedded file
// descriptor
struct EmbeddedCompletionFile
{
    const char* name;
    std::string_view content;

    EmbeddedCompletionFile(const char* fileName, std::string_view fileContent)
        : name(fileName), content(fileContent)
    {
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

[[nodiscard]] std::array<EmbeddedCompletionFile, EmbeddedCompletionFileCount>
embeddedCompletionFiles();

}  // namespace beez::cli
