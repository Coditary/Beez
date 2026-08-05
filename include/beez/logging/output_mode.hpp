#pragma once

#include <cstdint>

namespace beez::logging
{

enum class OutputMode : std::uint8_t
{
    Clean,
    Verbose,
    // Future: Tui — stack-based terminal UI with bidirectional growth
};

}  // namespace beez::logging
