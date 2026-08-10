#include "beez/core/config/ui/progress_detail.hpp"

#include "beez/core/model/step.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace beez::core
{

std::string truncateForDisplay(std::string_view text, std::size_t maxLength)
{
    std::string normalized;
    normalized.reserve(text.size());
    for (const char Character : text)
    {
        if (Character == '\n' || Character == '\r')
        {
            normalized += ' ';
            continue;
        }
        normalized += Character;
    }

    if (normalized.size() <= maxLength)
    {
        return normalized;
    }

    if (maxLength <= 3)
    {
        return normalized.substr(0, maxLength);
    }

    return normalized.substr(0, maxLength - 3) + "...";
}

std::string stepProgressDetail(const Step& step)
{
    if (step.description.has_value() && !step.description->empty())
    {
        return *step.description;
    }

    return step.name;
}

}  // namespace beez::core
