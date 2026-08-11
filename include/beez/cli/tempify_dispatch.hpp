#pragma once

#include <string>
#include <vector>

namespace beez::cli
{

[[nodiscard]] bool isInitMode(int argc, const char* const* argv);

[[nodiscard]] std::vector<std::string> collectInitArgs(int argc, const char* const* argv);

[[nodiscard]] int runTempifyInitMode(const std::vector<std::string>& args);

}  // namespace beez::cli
