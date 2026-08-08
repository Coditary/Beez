#pragma once

#include <array>
#include <string_view>

namespace beez::cli
{

struct EmbeddedCompletionFile
{
    const char* name;
    std::string_view content;
};

[[nodiscard]] std::array<EmbeddedCompletionFile, 5> embeddedCompletionFiles();

}  // namespace beez::cli
