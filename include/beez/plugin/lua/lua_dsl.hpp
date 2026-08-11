#pragma once

#include "beez/core/config/settings/settings.hpp"
#include "beez/core/reqpack/types.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/contract/plugin.hpp"

#include <memory>
#include <string>

namespace beez::plugin::lua
{

class LuaDslLoader : public IDslLoader
{
  public:
    LuaDslLoader();
    ~LuaDslLoader() override;

    LuaDslLoader(const LuaDslLoader&) = delete;
    LuaDslLoader& operator=(const LuaDslLoader&) = delete;
    LuaDslLoader(LuaDslLoader&&) = delete;
    LuaDslLoader& operator=(LuaDslLoader&&) = delete;

    bool load(const core::Context& context, core::Registry& registry) override;
    void setGcThroughputMode(bool enable) override;
    void releaseState();

    [[nodiscard]] const core::BeezSettings& buildSettings() const;

    [[nodiscard]] const core::ReqPackManifest& reqpackManifest() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class LuaDslPlugin : public IPlugin
{
  public:
    [[nodiscard]] std::string name() const override;
    void registerCapabilities(PluginHost& host) override;
};

}  // namespace beez::plugin::lua
