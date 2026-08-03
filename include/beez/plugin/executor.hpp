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
    virtual ~IExecutor() = default;
    virtual int execute(const std::string& command, const core::Context& context) = 0;
};

}  // namespace beez::plugin
