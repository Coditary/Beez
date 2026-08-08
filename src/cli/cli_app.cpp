#include "beez/cli/cli_app.hpp"
#include "beez/cli/parsed_options.hpp"

#include "beez/core/phase_argument_parser.hpp"
#include "beez/version.hpp"

#include <CLI/CLI.hpp>

#include <lua.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace beez::cli
{

namespace
{

constexpr int MaxThreadCount = 1024;

[[nodiscard]] std::string luaVersionString()
{
    std::ostringstream stream;
    stream << LUA_VERSION_MAJOR << '.' << LUA_VERSION_MINOR << '.' << LUA_VERSION_RELEASE;
    return stream.str();
}

}  // namespace

std::string CliApp::helpText()
{
    std::ostringstream stream;
    stream << "Beez - Build Everything Easy (" << version::VersionString << ")\n\n";
    stream << "Usage: beez [target] [core-options] [-- user-options]\n\n";
    stream << "Options:\n";
    stream << "  -h, --help       Display this help and exit\n";
    stream << "  -v, --version    Display the installed Beez and Lua version\n";
    stream << "      --verbose    Enable verbose logging (Ninja-style)\n";
    stream << "      --dry-run    Build the graph without executing Lua scripts\n";
    stream << "      --no-cache   Disable step and success caching\n";
    stream << "      --clean-cache Remove .cache/ before running\n";
    stream << "      --install-completion Register shell tab completion (no make install-beez "
              "needed)\n";
    stream << "  -j, --threads N  Maximum worker threads (default: CPU cores)\n";
    stream << "      --list TEXT  List registered entities (tasks, workflows, steps, phases, "
              "phase-scopes)\n";
    stream << "  -p, --phase TEXT Run a phase (phase[:scope1,scope2] or phase[\"scope\"])\n";
    stream << "  -s, --step TEXT  Run a single step by name\n";
    return stream.str();
}

std::string CliApp::versionText()
{
    std::ostringstream stream;
    stream << "Beez " << version::VersionString << '\n';
    stream << "Lua " << luaVersionString();
    return stream.str();
}

CliParseResult CliApp::parse(int argc, const char* const* argv)
{
    CLI::App app {"Beez"};
    app.allow_extras();
    app.set_help_flag("", "");
    app.set_version_flag("", "");

    ParsedOptions options;
    std::string listKind;
    std::string phaseArgument;
    std::string stepName;

    app.add_flag_callback("-h,--help", []() {}, "Display this help and exit");

    app.add_flag_callback("-v,--version", []() {}, "Display the installed Beez and Lua version");

    bool disableCache = false;
    bool cleanCache = false;
    bool installCompletion = false;
    std::size_t threadCount = 0;

    app.add_flag("--verbose", options.verbose, "Enable verbose logging (Ninja-style)");
    app.add_flag("--dry-run", options.dryRun, "Build the graph without executing Lua scripts");
    app.add_flag("--no-cache", disableCache, "Disable step and success caching");
    app.add_flag("--clean-cache", cleanCache, "Remove .cache/ before running");
    app.add_flag("--install-completion",
                 installCompletion,
                 "Register shell tab completion in ~/.zshrc / ~/.bashrc");
    app.add_option("-j,--threads", threadCount, "Maximum worker threads (default: CPU cores)")
        ->check(CLI::Range(1, MaxThreadCount));
    app.add_option("--list", listKind, "List registered entities (tasks, workflows, steps, phases)")
        ->check(CLI::IsMember({"tasks", "workflows", "steps", "phases", "phase-scopes"}));
    app.add_option(
        "-p,--phase", phaseArgument, "Run a phase (phase[:scope1,scope2] or phase[\"scope\"])");
    app.add_option("-s,--step", stepName, "Run a single step by name");
    app.add_option("target", options.target, "Task or workflow to run");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& error)
    {
        CliParseResult result;
        result.reason = CliExitReason::Error;
        result.exitCode = error.get_exit_code();
        return result;
    }

    if (app.count("-h") > 0)
    {
        return {.reason = CliExitReason::Help, .exitCode = 0};
    }

    if (app.count("-v") > 0)
    {
        return {.reason = CliExitReason::Version, .exitCode = 0};
    }

    if (!listKind.empty())
    {
        options.listKind = listKind;
    }

    if (!phaseArgument.empty())
    {
        const auto ParsedPhase = core::parsePhaseArgument(phaseArgument);
        if (!ParsedPhase)
        {
            return {.reason = CliExitReason::Error, .exitCode = 1};
        }
        options.phaseRequest = ParsedPhase;
    }

    if (!stepName.empty())
    {
        options.stepName = stepName;
    }

    const auto& remaining = app.remaining();
    const auto Separator = std::ranges::find(remaining, "--");
    if (Separator != remaining.end())
    {
        options.userOptions.assign(Separator + 1, remaining.end());
    }

    const bool HasRunTarget = options.target.has_value() || options.phaseRequest.has_value() ||
                              options.stepName.has_value() || options.listKind.has_value() ||
                              cleanCache || installCompletion;

    if (!HasRunTarget)
    {
        return {.reason = CliExitReason::Help, .exitCode = 1};
    }

    options.cleanCache = cleanCache;
    options.installCompletion = installCompletion;
    options.enableCache = !disableCache;
    if (threadCount > 0)
    {
        options.maxThreads = threadCount;
    }

    return {.reason = CliExitReason::Continue, .options = std::move(options), .exitCode = 0};
}

}  // namespace beez::cli
