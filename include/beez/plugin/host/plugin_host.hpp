#pragma once

#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/contract/executor.hpp"
#include "beez/plugin/contract/plugin.hpp"

#include <memory>
#include <vector>

namespace beez::core
{
class Context;
class Registry;
}  // namespace beez::core

namespace beez::plugin
{

class PluginHost
{
  public:
    void addPlugin(std::unique_ptr<IPlugin> plugin);
    void initialize(core::Registry& registry, core::Context& context);

    [[nodiscard]] IExecutor* executor() const;
    [[nodiscard]] IDslLoader* dslLoader() const;

    void setExecutor(std::unique_ptr<IExecutor> executor);
    void setDslLoader(std::unique_ptr<IDslLoader> dslLoader);

  private:
    std::vector<std::unique_ptr<IPlugin>> plugins_;
    std::unique_ptr<IExecutor> executor_;
    std::unique_ptr<IDslLoader> dslLoader_;
};

}  // namespace beez::plugin
