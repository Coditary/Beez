#include "beez/cli/parsing/help_text.hpp"

#include "beez/version.hpp"

#include <lua.h>

#include <sstream>
#include <string>

namespace beez::cli
{

namespace
{

[[nodiscard]] std::string luaVersionString()
{
    std::ostringstream stream;
    stream << LUA_VERSION_MAJOR << '.' << LUA_VERSION_MINOR << '.' << LUA_VERSION_RELEASE;
    return stream.str();
}

}  // namespace

std::string helpText()
{
    std::ostringstream stream;
    stream << "Beez - Build Everything Easy (" << version::VersionString << ")\n\n";
    stream << "Usage: beez [target] [core-options] [-- user-options]\n\n";
    stream << "Options:\n";
    stream
        << "      --init         Run Tempify template scaffolding (beez --init <tempify-args>)\n";
    stream << "  -h, --help       Display this help and exit\n";
    stream << "  -v, --version    Display the installed Beez and Lua version\n";
    stream << "      --verbose    Enable verbose logging (Ninja-style)\n";
    stream << "      --silent     Suppress all run output (exit code only)\n";
    stream << "      --error      Print errors only, no progress or success summary\n";
    stream << "      --log-file PATH  Write run log to PATH (default: .cache/logs/latest.log)\n";
    stream << "      --no-log-file    Disable run log file output\n";
    stream << "      --dry-run    Build the graph without executing Lua scripts\n";
    stream << "      --no-cache   Disable step and success caching\n";
    stream << "      --show-config Show merged active configuration and exit\n";
    stream << "      --config-options [PATH]  List config keys or allowed values for PATH\n";
    stream << "      --clean-cache Remove .cache/ before running\n";
    stream << "      --update      Apply cache storage updates for the active configuration\n";
    stream << "      --install     Install reqpack dependencies declared in build.lua\n";
    stream << "      --install-completion Register shell tab completion (no make install-beez "
              "needed)\n";
    stream << "  -j, --threads N  Maximum worker threads (default: CPU cores)\n";
    stream << "\nConfiguration:\n";
    stream << "  User defaults: ~/.config/beez/config.lua (return a settings table)\n";
    stream << "  Project overrides: beez.config({ ... }) in build.lua\n";
    stream << "  CLI flags override both.\n";
    stream << "      --list TEXT  List registered entities (tasks, workflows, steps, phases)\n";
    stream << "  -p, --phase TEXT Run a phase (phase[:scope1,scope2] or phase[\"scope\"])\n";
    stream << "  -s, --step TEXT  Run a step by name (plugin:step, org/plugin:step, optional "
              "@version)\n";
    return stream.str();
}

std::string versionText()
{
    std::ostringstream stream;
    stream << "Beez " << version::VersionString << '\n';
    stream << "Lua " << luaVersionString();
    return stream.str();
}

}  // namespace beez::cli
