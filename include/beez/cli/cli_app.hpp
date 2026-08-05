#pragma once

#include "beez/cli/parsed_options.hpp"

#include <string>

namespace beez::cli
{

class CliApp
{
  public:
    [[nodiscard]] static CliParseResult parse(int argc, const char* const* argv);
    [[nodiscard]] static std::string helpText();
    [[nodiscard]] static std::string versionText();
};

}  // namespace beez::cli
