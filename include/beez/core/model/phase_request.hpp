#pragma once

#include <string>
#include <vector>

namespace beez::core
{

struct PhaseRequest
{
    std::string phase;
    std::vector<std::string> scopes;
};

}  // namespace beez::core
