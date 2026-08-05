#pragma once

#include "beez/core/phase_request.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace beez::cli
{

struct ParsedOptions
{
    std::optional<std::string> target;
    std::optional<core::PhaseRequest> phaseRequest;
    std::optional<std::string> stepName;
    std::optional<std::string> listKind;
    bool verbose = false;
    bool dryRun = false;
    std::vector<std::string> userOptions;
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
