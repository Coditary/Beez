#include "beez/plugin/lua/api/sys/user.hpp"

#include <cstdlib>
#include <string>
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
    if (const char* user = std::getenv("USER"); user != nullptr && user[0] != '\0')
    {
        return user;
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
    if (const char* logName = std::getenv("LOGNAME"); logName != nullptr && logName[0] != '\0')
    {
        return logName;
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe) -- POSIX login name lookup for build DSL
    if (const char* login = getlogin(); login != nullptr && login[0] != '\0')
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
