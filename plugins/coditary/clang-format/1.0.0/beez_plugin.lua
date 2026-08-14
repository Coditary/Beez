local defaults = require("src.defaults")
local step_config = require("src.step_config")

-- Beez clang-format plugin
--
-- Steps (after load):
--   qa_check       — phase qa, scope code
--   format_apply   — phase format, scope code
--
-- Optional overrides from build.lua (after reqpack):
--   configure_step("qa_check", { format_rev = "2", patterns = { ... } })
--
-- reqpack {
--     beez = {
--         {
--             name = "coditary/clang-format",
--             path = "./plugins/coditary/clang-format",
--             version = "1.0.0",
--         },
--     },
-- }

plugin("clang-format", {
    version = "1.0.0",
    description = "Incremental clang-format check and apply",
    organization = "coditary",

    steps = {
        qa_check = {
            phase = "quality",
            scope = "code",
            input = defaults.patterns,
            description = "clang-format check (incremental)",
            config = step_config.defaults(),
            run = function(ctx)
                return require("src.runner").check(ctx)
            end,
        },

        format_apply = {
            phase = "quality",
            scope = "code",
            mutate = defaults.patterns,
            description = "Apply clang-format (incremental)",
            config = step_config.defaults(),
            run = function(ctx)
                return require("src.runner").apply(ctx)
            end,
        },
    },
})
