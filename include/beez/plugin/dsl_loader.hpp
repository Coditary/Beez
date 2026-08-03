#pragma once

namespace beez::core
{
class Context;
class Registry;
}  // namespace beez::core

namespace beez::plugin
{

class IDslLoader
{
  public:
    virtual ~IDslLoader() = default;
    virtual bool load(const core::Context& context, core::Registry& registry) = 0;
};

}  // namespace beez::plugin
