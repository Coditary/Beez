#pragma once

namespace beez::cli
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays) -- CLI entry point
[[nodiscard]] int run(int argc, const char* argv[]);

}  // namespace beez::cli
