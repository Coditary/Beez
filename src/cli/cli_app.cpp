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
    stream << "      --log-file PATH  Write run log to PATH (default: .cache/logs/latest.log)\n";
    stream << "      --no-log-file    Disable run log file output\n";
    stream << "      --dry-run    Build the graph without executing Lua scripts\n";
    stream << "      --no-cache   Disable step and success caching\n";
    stream << "      --show-config Show merged active configuration and exit\n";
    stream << "      --config-options [PATH]  List config keys or allowed values for PATH\n";
    stream << "      --clean-cache Remove .cache/ before running\n";
    stream << "      --update      Apply cache storage updates for the active configuration\n";
    stream << "      --install-completion Register shell tab completion (no make install-beez "
              "needed)\n";
    stream << "  -j, --threads N  Maximum worker threads (default: CPU cores)\n";
    stream << "\nConfiguration:\n";
    stream << "  User defaults: ~/.config/beez/config.lua (return a settings table)\n";
    stream << "      --config PATH  Load settings from PATH instead of the user config file\n";
    stream << "      --build-file PATH  Load the build script from PATH (default: ./build.lua)\n";
    stream << "  Project overrides: beez.config({ ... }) in build.lua\n";
    stream << "  CLI flags override both.\n";
    stream << "      --list TEXT  List registered entities (tasks, workflows, steps, phases)\n";
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
    bool updateCache = false;
    bool installCompletion = false;
    bool showConfig = false;
    bool noLogFile = false;
    std::string logFile;
    std::string configFile;
    std::string buildFile;
    std::string configOptionsPath;
    std::string completeConfigOptionsPrefix;
    std::string dumpCompletionShell;

    app.add_flag("--verbose", options.verbose, "Enable verbose logging (Ninja-style)");
    app.add_option(
        "--log-file", logFile, "Write run log to PATH (default: .cache/logs/latest.log)");
    app.add_flag("--no-log-file", noLogFile, "Disable run log file output");
    app.add_option(
        "--config", configFile, "Load settings from PATH instead of ~/.config/beez/config.lua");
    app.add_option(
        "--build-file", buildFile, "Load the build script from PATH (default: ./build.lua)");
    app.add_flag("--dry-run", options.dryRun, "Build the graph without executing Lua scripts");
    app.add_flag("--no-cache", disableCache, "Disable step and success caching");
    app.add_flag("--show-config", showConfig, "Show merged active configuration and exit");
    auto* configOptionsOption =
        app.add_option("--config-options",
                       configOptionsPath,
                       "List config keys or allowed values for a dotted path");
    configOptionsOption->expected(0, 1);
    auto* completeConfigOptionsOption =
        app.add_option("--complete-config-options",
                       completeConfigOptionsPrefix,
                       "List config option paths for shell tab completion");
    completeConfigOptionsOption->expected(0, 1);
    app.add_option(
           "--dump-completion", dumpCompletionShell, "Print shell completion script (bash or zsh)")
        ->check(CLI::IsMember({"bash", "zsh"}));
    app.add_flag("--clean-cache", cleanCache, "Remove .cache/ before running");
    app.add_flag(
        "--update", updateCache, "Apply cache storage updates for the active configuration");
    app.add_flag("--install-completion",
                 installCompletion,
                 "Register shell tab completion in ~/.zshrc / ~/.bashrc");
    std::size_t threadCount = 0;
    app.add_option("-j,--threads", threadCount, "Maximum worker threads (default: CPU cores)")
        ->check(CLI::Range(1, MaxThreadCount));
    app.add_option("--list", listKind, "List registered entities (tasks, workflows, steps, phases)")
        ->check(CLI::IsMember({"tasks", "workflows", "steps", "phases"}));
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

    const bool HasRunTarget =
        options.target.has_value() || options.phaseRequest.has_value() ||
        options.stepName.has_value() || options.listKind.has_value() || cleanCache || updateCache ||
        installCompletion || showConfig || app.count("--config-options") > 0 ||
        app.count("--complete-config-options") > 0 || app.count("--dump-completion") > 0;

    if (!HasRunTarget)
    {
        return {.reason = CliExitReason::Help, .exitCode = 1};
    }

    options.cleanCache = cleanCache;
    options.updateCache = updateCache;
    options.installCompletion = installCompletion;
    options.showConfig = showConfig;
    options.configOptions = app.count("--config-options") > 0;
    options.configOptionsPath = configOptionsPath;
    options.completeConfigOptions = app.count("--complete-config-options") > 0;
    options.completeConfigOptionsPrefix = completeConfigOptionsPrefix;
    options.dumpCompletion = app.count("--dump-completion") > 0;
    options.dumpCompletionShell = dumpCompletionShell;
    options.enableCache = !disableCache;
    if (threadCount > 0)
    {
        options.maxThreads = threadCount;
    }

    options.noLogFile = noLogFile;
    if (!logFile.empty())
    {
        options.logFile = logFile;
    }
    if (!configFile.empty())
    {
        options.configFile = configFile;
    }
    if (!buildFile.empty())
    {
        options.buildFile = buildFile;
    }

    return {.reason = CliExitReason::Continue, .options = std::move(options), .exitCode = 0};
}

}  // namespace beez::cli
