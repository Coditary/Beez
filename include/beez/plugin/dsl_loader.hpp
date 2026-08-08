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
    IDslLoader() = default;
    virtual ~IDslLoader() = default;
    IDslLoader(const IDslLoader&) = delete;
    IDslLoader& operator=(const IDslLoader&) = delete;
    IDslLoader(IDslLoader&&) = delete;
    IDslLoader& operator=(IDslLoader&&) = delete;

    virtual bool load(const core::Context& context, core::Registry& registry) = 0;

    virtual void setGcThroughputMode(bool enable)
    {
        (void)enable;
    }
};

}  // namespace beez::plugin
