#include "beez/cli/commands/list.hpp"

#include "beez/cli/presentation/entity_table.hpp"
#include "beez/core/registry/registry.hpp"

#include <iostream>
#include <string>

namespace beez::cli
{

int runListCommand(const core::Registry& registry, const std::string& kind)
{
    std::cout << formatEntityList(registry, kind);
    return 0;
}

}  // namespace beez::cli
