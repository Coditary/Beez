#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

#include <gtest/gtest.h>

#include <memory>

TEST(PluginHostTest, HasNoCapabilitiesBeforeInitialization)
{
    const beez::plugin::PluginHost Host;

    EXPECT_EQ(Host.executor(), nullptr);
    EXPECT_EQ(Host.dslLoader(), nullptr);
}

TEST(PluginHostTest, ShellPluginRegistersExecutor)
{
    beez::core::Registry registry;
    beez::core::Context context;
    beez::plugin::PluginHost pluginHost;

    pluginHost.addPlugin(std::make_unique<beez::plugin::shell::ShellPlugin>());
    pluginHost.initialize(registry, context);

    EXPECT_NE(pluginHost.executor(), nullptr);
    EXPECT_EQ(pluginHost.dslLoader(), nullptr);
}

TEST(PluginHostTest, LuaPluginRegistersDslLoader)
{
    beez::core::Registry registry;
    beez::core::Context context;
    beez::plugin::PluginHost pluginHost;

    pluginHost.addPlugin(std::make_unique<beez::plugin::lua::LuaDslPlugin>());
    pluginHost.initialize(registry, context);

    EXPECT_EQ(pluginHost.executor(), nullptr);
    EXPECT_NE(pluginHost.dslLoader(), nullptr);
}

TEST(PluginHostTest, BothPluginsRegisterTheirCapabilities)
{
    beez::core::Registry registry;
    beez::core::Context context;
    beez::plugin::PluginHost pluginHost;

    pluginHost.addPlugin(std::make_unique<beez::plugin::shell::ShellPlugin>());
    pluginHost.addPlugin(std::make_unique<beez::plugin::lua::LuaDslPlugin>());
    pluginHost.initialize(registry, context);

    EXPECT_NE(pluginHost.executor(), nullptr);
    EXPECT_NE(pluginHost.dslLoader(), nullptr);
}

TEST(PluginHostTest, PluginNamesAreStable)
{
    const beez::plugin::shell::ShellPlugin ShellPlugin;
    const beez::plugin::lua::LuaDslPlugin LuaPlugin;

    EXPECT_EQ(ShellPlugin.name(), "shell");
    EXPECT_EQ(LuaPlugin.name(), "lua_dsl");
}
