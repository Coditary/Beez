#include "beez/plugin/lua/api/char/quote.hpp"

#include <string>

namespace beez::plugin::lua
{

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (const char character : value)
    {
        if (character == '\'')
        {
            quoted += "'\\''";
            continue;
        }

        quoted += character;
    }

    quoted += "'";
    return quoted;
}

}  // namespace beez::plugin::lua
