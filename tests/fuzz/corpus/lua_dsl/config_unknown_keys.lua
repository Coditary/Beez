beez.config({
    typo_section = {
        enabled = true,
    },
    performance = {
        not_a_real_key = 42,
    },
})

task("hello", "echo config-unknown-warning > out.txt")
