#include "beez/cli/list_formatter.hpp"

#include "beez/core/registry.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace beez::cli
{

std::vector<std::string> collectEntityNames(const core::Registry& registry, const std::string& kind)
{
    std::vector<std::string> names;
    if (kind == "tasks")
    {
        names.reserve(registry.tasks().size());
        for (const auto& [name, task] : registry.tasks())
        {
            (void)task;
            names.push_back(name);
        }
    }
    else if (kind == "workflows")
    {
        names.reserve(registry.workflows().size());
        for (const auto& [name, workflow] : registry.workflows())
        {
            (void)workflow;
            names.push_back(name);
        }
    }
    else if (kind == "steps")
    {
        names.reserve(registry.steps().size());
        for (const auto& [name, step] : registry.steps())
        {
            (void)step;
            names.push_back(name);
        }
    }

    std::ranges::sort(names);
    return names;
}

std::string formatEntityList(const std::string& kind, const std::vector<std::string>& names)
{
    std::ostringstream stream;
    stream << kind << ":\n";
    for (const auto& name : names)
    {
        stream << "  " << name << '\n';
    }
    return stream.str();
}

}  // namespace beez::cli
