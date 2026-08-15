#include "beez/plugin/lua/api/char/quote.hpp"

#include <string>

namespace beez::plugin::lua
{

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (const char Character : value)
    {
        if (Character == '\'')
        {
            quoted += "'\\''";
            continue;
        }

        quoted += Character;
    }

    quoted += "'";
    return quoted;
}

}  // namespace beez::plugin::lua
