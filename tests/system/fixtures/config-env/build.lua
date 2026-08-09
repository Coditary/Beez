beez.config({
    env = {
        vars = {
            BEEZ_SYSTEM_ENV_TEST = "fixture-value",
        },
    },
})

task("check", "test \"$BEEZ_SYSTEM_ENV_TEST\" = fixture-value")
