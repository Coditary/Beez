#include "beez/plugin/lua/api/sys/user.hpp"

#include <cstdlib>
#include <string>
#include <string_view>
#include <unistd.h>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::string currentUserName()
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
    if (const char* user = std::getenv("USER"); user != nullptr && !std::string_view(user).empty())
    {
        return user;
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
    if (const char* logName = std::getenv("LOGNAME");
        logName != nullptr && !std::string_view(logName).empty())
    {
        return logName;
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe) -- POSIX login name lookup for build DSL
    if (const char* login = getlogin(); login != nullptr && !std::string_view(login).empty())
    {
        return login;
    }

    return "unknown";
}

}  // namespace

void bindUser(sol::table& sysTable)
{
    sysTable["user"] = []() -> std::string { return currentUserName(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
