#pragma once

#include <string>

namespace beez::core
{

// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
struct PhaseInvocation
{
    std::string phase;
    std::string scope;
};

}  // namespace beez::core
