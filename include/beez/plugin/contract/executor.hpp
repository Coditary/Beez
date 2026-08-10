#pragma once

#include <string>

namespace beez::core
{
class Context;
}  // namespace beez::core

namespace beez::plugin
{

class IExecutor
{
  public:
    IExecutor() = default;
    virtual ~IExecutor() = default;
    IExecutor(const IExecutor&) = delete;
    IExecutor& operator=(const IExecutor&) = delete;
    IExecutor(IExecutor&&) = delete;
    IExecutor& operator=(IExecutor&&) = delete;

    virtual int execute(const std::string& command,
                        const core::Context& context,
                        std::string* capturedOutput = nullptr) = 0;
};

}  // namespace beez::plugin
