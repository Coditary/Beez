#include "beez/cli/commands/run.hpp"

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/cli/presentation/name_suggestion.hpp"
#include "beez/core/expected.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/logging/console/output_mode.hpp"

#include <iostream>
#include <string>

namespace beez::cli
{

namespace
{

void printOrchestratorError(core::OrchestratorError error, logging::OutputMode outputMode)
{
    if (!logging::writesCliErrorsToConsole(outputMode))
    {
        return;
    }

    std::cerr << "Error: " << core::toString(error) << '\n';
}

void printTargetNotFoundHint(const core::Registry& registry,
                             const std::string& target,
                             logging::OutputMode outputMode)
{
    if (!logging::writesCliErrorsToConsole(outputMode))
    {
        return;
    }

    if (const auto Suggestion = suggestSimilarName(target, collectRunnableNames(registry)))
    {
        std::cerr << formatDidYouMean(*Suggestion) << '\n';
    }
}

[[nodiscard]] int finishRunResult(const Expected<int, core::OrchestratorError>& result,
                                  logging::OutputMode outputMode)
{
    if (!result)
    {
        printOrchestratorError(result.error(), outputMode);
        return 1;
    }

    return result.value();
}

}  // namespace

int runOrchestratorCommand(core::Orchestrator& orchestrator,
                           const core::Registry& registry,
                           const ParsedOptions& options,
                           logging::OutputMode outputMode)
{
    if (options.stepName.has_value())
    {
        return finishRunResult(orchestrator.runStep(*options.stepName), outputMode);
    }

    if (options.phaseRequest.has_value())
    {
        return finishRunResult(orchestrator.runPhase(*options.phaseRequest), outputMode);
    }

    if (!options.target.has_value())
    {
        return 1;
    }

    const auto RunResult = orchestrator.run(*options.target);
    if (!RunResult)
    {
        printOrchestratorError(RunResult.error(), outputMode);
        if (RunResult.error() == core::OrchestratorError::NotFound)
        {
            printTargetNotFoundHint(registry, *options.target, outputMode);
        }
        return 1;
    }

    return RunResult.value();
}

}  // namespace beez::cli
