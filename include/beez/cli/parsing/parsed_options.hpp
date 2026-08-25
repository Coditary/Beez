#pragma once

#include "beez/core/model/phase_request.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace beez::cli
{

struct ParsedOptions
{
    std::optional<std::size_t> maxThreads;
    std::vector<std::string> userOptions;
    std::vector<std::string> defines;
    std::string configOptionsPath;
    std::string completeConfigOptionsPrefix;
    std::string dumpCompletionShell;
    std::optional<std::string> target;
    std::optional<std::string> stepName;
    std::optional<std::string> listKind;
    std::optional<std::string> profile;
    std::optional<std::filesystem::path> logFile;
    std::optional<std::filesystem::path> linkPath;
    std::optional<core::PhaseRequest> phaseRequest;
    bool fromBridge = false;
    bool fromGlobal = false;
    bool verbose = false;
    bool silent = false;
    bool errorsOnly = false;
    bool dryRun = false;
    bool cleanCache = false;
    bool updateCache = false;
    bool installCompletion = false;
    bool installDependencies = false;
    bool showConfig = false;
    bool configOptions = false;
    bool completeConfigOptions = false;
    bool dumpCompletion = false;
    bool enableCache = true;
    bool noLogFile = false;
};

enum class CliExitReason : std::uint8_t
{
    Continue,
    Help,
    Version,
    Error,
};

struct CliParseResult
{
    CliExitReason reason = CliExitReason::Continue;
    ParsedOptions options {};
    int exitCode = 0;
};

}  // namespace beez::cli
