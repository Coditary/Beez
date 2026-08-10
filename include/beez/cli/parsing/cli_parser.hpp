#pragma once

#include "beez/cli/parsing/parsed_options.hpp"

namespace beez::cli
{

class CliParser
{
  public:
    [[nodiscard]] static CliParseResult parse(int argc, const char* const* argv);
};

}  // namespace beez::cli
