#include "lua_step_config.hpp"

#include "beez/core/context.h"
#include "beez/core/step_config.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

sol::table shallowCopyTable(const std::shared_ptr<sol::state>& luaState, const sol::table& source)
{
    sol::table copy = luaState->create_table();
    source.for_each([&copy](const sol::object& key, const sol::object& value)
                    { copy[key] = value; });
    return copy;
}

sol::table mergeTables(const std::shared_ptr<sol::state>& luaState,
                       const sol::table& base,
                       const sol::table& overlay)
{
    sol::table merged = shallowCopyTable(luaState, base);
    overlay.for_each([&merged](const sol::object& key, const sol::object& value)
                     { merged[key] = value; });
    return merged;
}

class LuaStepConfig final : public core::StepConfig
{
  public:
    LuaStepConfig(std::shared_ptr<sol::state> luaState, std::function<sol::table()> lazyBuilder)
        : luaState_(std::move(luaState)), lazyBuilder_(std::move(lazyBuilder))
    {
    }

    [[nodiscard]] bool empty() const override
    {
        return !lazyBuilder_;
    }

    [[nodiscard]] core::StepConfigPtr mergedWith(const core::StepConfigPtr& overlay) const override
    {
        const auto* overlayConfig = dynamic_cast<const LuaStepConfig*>(overlay.get());
        if (overlayConfig == nullptr)
        {
            return overlay;
        }

        return std::make_shared<LuaStepConfig>(
            luaState_,
            [luaState = luaState_,
             baseBuilder = lazyBuilder_,
             overlayBuilder = overlayConfig->lazyBuilder_]() -> sol::table
            {
                const sol::table BaseTable = baseBuilder();
                const sol::table OverlayTable = overlayBuilder();
                return mergeTables(luaState, BaseTable, OverlayTable);
            });
    }

    [[nodiscard]] sol::table materialize() const
    {
        if (!cachedTable_.has_value())
        {
            cachedTable_ = lazyBuilder_();
        }

        return cachedTable_.value();
    }

  private:
    std::shared_ptr<sol::state> luaState_;
    std::function<sol::table()> lazyBuilder_;
    mutable std::optional<sol::table> cachedTable_;
};

}  // namespace

core::StepConfigPtr makeLuaStepConfig(const std::shared_ptr<sol::state>& luaState,
                                      const sol::table& configTable)
{
    return std::make_shared<LuaStepConfig>(luaState,
                                           [luaState, configTable]() -> sol::table
                                           { return shallowCopyTable(luaState, configTable); });
}

sol::table bindStepContext(const std::shared_ptr<sol::state>& luaState,
                           const core::Context& context)
{
    sol::table stepContext = luaState->create_table();
    stepContext["project_root"] = context.projectRoot().string();
    stepContext["get_config"] = [&context]() -> sol::object
    {
        const core::StepConfigPtr StepConfig = context.getConfig();
        if (StepConfig == nullptr || StepConfig->empty())
        {
            return sol::lua_nil;
        }

        const auto* luaConfig = dynamic_cast<const LuaStepConfig*>(StepConfig.get());
        if (luaConfig == nullptr)
        {
            return sol::lua_nil;
        }

        return luaConfig->materialize();
    };

    return stepContext;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
