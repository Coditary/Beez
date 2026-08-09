beez.config({
    env = {
        load_dotenv = true,
        dotenv_overrides_system = true,
        vars = {
            FUZZ_TOKEN = "secret",
            BUILD_TYPE = "Debug",
        },
        hash_vars = "PATH,HOME,FUZZ_TOKEN",
        ignore_vars_for_hashing = "TMPDIR",
        mask_secrets = "FUZZ_TOKEN,SECRET",
    },
})

step({
    name = "use-env",
    phase = "demo",
    scope = "default",
    run = "echo ${BEEZ_ENV:-missing}",
})
