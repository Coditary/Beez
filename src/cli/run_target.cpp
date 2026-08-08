#include "beez/cli/run_target.hpp"

#include "beez/cli/list_formatter.hpp"
#include "beez/cli/name_suggestion.hpp"
#include "beez/cli/parsed_options.hpp"
#include "beez/core/expected.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"

#include <iostream>
#include <string>

namespace beez::cli
{

namespace
{

void printOrchestratorError(core::OrchestratorError error)
{
    std::cerr << "Error: " << core::toString(error) << '\n';
}

void printTargetNotFoundHint(const core::Registry& registry, const std::string& target)
{
    if (const auto Suggestion = suggestSimilarName(target, collectRunnableNames(registry)))
    {
        std::cerr << formatDidYouMean(*Suggestion) << '\n';
    }
}

[[nodiscard]] int finishRunResult(const Expected<int, core::OrchestratorError>& result)
{
    if (!result)
    {
        printOrchestratorError(result.error());
        return 1;
    }

    return result.value();
}

}  // namespace

int runParsedInvocation(core::Orchestrator& orchestrator,
                        const core::Registry& registry,
                        const ParsedOptions& options)
{
    if (options.listKind.has_value())
    {
        std::cout << formatEntityList(registry, *options.listKind);
        return 0;
    }

    if (options.stepName.has_value())
    {
        return finishRunResult(orchestrator.runStep(*options.stepName));
    }

    if (options.phaseRequest.has_value())
    {
        return finishRunResult(orchestrator.runPhase(*options.phaseRequest));
    }

    if (!options.target.has_value())
    {
        return 1;
    }

    const auto RunResult = orchestrator.run(*options.target);
    if (!RunResult)
    {
        printOrchestratorError(RunResult.error());
        if (RunResult.error() == core::OrchestratorError::NotFound)
        {
            printTargetNotFoundHint(registry, *options.target);
        }
        return 1;
    }

    return RunResult.value();
}

}  // namespace beez::cli
