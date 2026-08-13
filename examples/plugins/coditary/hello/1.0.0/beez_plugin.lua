-- Example Beez plugin: hello
--
-- Install into the plugin cache (adjust paths as needed):
--   mkdir -p ~/.cache/beez/plugins/coditary/hello/1.0.0
--   cp examples/plugins/coditary/hello/1.0.0/* ~/.cache/beez/plugins/coditary/hello/1.0.0/
--
-- Use in build.lua:
--   require {
--       beez = {
--           { name = "hello", version = "1.0.0" },
--       },
--   }

plugin("hello", {
    version = "1.0.0",
    description = "Minimal example plugin with shell and Lua steps",
    organization = "coditary",

    steps = {
        greet = {
            phase = "generate",
            scope = "demo",
            description = "Print a greeting via shell",
            run = "echo Hello from the hello plugin",
        },

        -- Lazy step definition: the table is built when the plugin loads.
        welcome = function()
            return {
                phase = "generate",
                scope = "demo",
                description = "Lazy-loaded step table",
                run = "echo Welcome from a lazy step",
            }
        end,

        -- Lua callback with config and require() from the plugin directory.
        check = {
            phase = "test",
            scope = "demo",
            description = "Lua run function using plugin-local helper module",
            config = { name = "Beez" },
            run = function(ctx)
                local helper = require("helper")
                local message = helper.greet(ctx.config.name)
                if message ~= "Hello, Beez!" then
                    return 1
                end
                return 0
            end,
        },
    },
})
