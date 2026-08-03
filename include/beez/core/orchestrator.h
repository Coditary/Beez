#pragma once

#include "beez/core/context.h"
#include "beez/core/expected.hpp"
#include "beez/core/registry.h"
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
    WorkflowNotImplemented,
    ExecutionFailed,
    BuildScriptNotFound,
    BuildScriptLoadFailed,
    ExecutorNotAvailable,
};

[[nodiscard]] const char* toString(OrchestratorError error);

class Orchestrator
{
  public:
    Orchestrator(Registry& registry, Context& context, plugin::PluginHost& pluginHost);

    [[nodiscard]] Expected<void, OrchestratorError> loadBuildScript();
    [[nodiscard]] Expected<int, OrchestratorError> run(const std::string& name);

  private:
    [[nodiscard]] Expected<int, OrchestratorError> runTask(const Task& task);
    [[nodiscard]] Expected<int, OrchestratorError> runWorkflow(const Workflow& workflow);

    Registry& registry_;
    Context& context_;
    plugin::PluginHost& pluginHost_;
};

}  // namespace beez::core
