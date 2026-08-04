#pragma once

#include "beez/core/context.h"
#include "beez/core/expected.hpp"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/registry.h"
#include "beez/core/step.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

#include <cstdint>
#include <string>

namespace beez::plugin
{
class PluginHost;
}  // namespace beez::plugin

namespace beez::core
{

enum class OrchestratorError : std::uint8_t
{
    NotFound,
    ExecutionFailed,
    BuildScriptNotFound,
    BuildScriptLoadFailed,
    ExecutorNotAvailable,
    InvalidPhaseRequest,
};

[[nodiscard]] const char* toString(OrchestratorError error);

class Orchestrator
{
  public:
    Orchestrator(Registry& registry, Context& context, plugin::PluginHost& pluginHost);

    [[nodiscard]] Expected<void, OrchestratorError> loadBuildScript();
    [[nodiscard]] Expected<int, OrchestratorError> run(const std::string& name);
    [[nodiscard]] Expected<int, OrchestratorError> runPhase(const PhaseRequest& request);
    [[nodiscard]] Expected<int, OrchestratorError> runStep(const std::string& name);

  private:
    [[nodiscard]] Expected<int, OrchestratorError> runTask(const Task& task);
    [[nodiscard]] Expected<int, OrchestratorError> runWorkflow(const Workflow& workflow);
    [[nodiscard]] Expected<int, OrchestratorError> runStepInstance(const Step& step);
    [[nodiscard]] Expected<int, OrchestratorError>
    runPhaseInvocation(const PhaseInvocation& invocation);
    [[nodiscard]] Expected<int, OrchestratorError> runShellCommand(const std::string& command);

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed kernel
    // dependencies
    Registry& registry_;
    Context& context_;
    plugin::PluginHost& pluginHost_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace beez::core
