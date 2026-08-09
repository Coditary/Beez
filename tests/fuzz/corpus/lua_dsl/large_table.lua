step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/**/*.o" },
    run = "mkdir -p build && echo object > build/main.o",
})
task("t01", "true")
task("t02", "true")
task("t03", "true")
task("t04", "true")
task("t05", "true")
task("t06", "true")
task("t07", "true")
task("t08", "true")
task("t09", "true")
task("t10", "true")
task("t11", "true")
task("t12", "true")
task("t13", "true")
task("t14", "true")
task("t15", "true")
task("t16", "true")
task("t17", "true")
task("t18", "true")
task("t19", "true")
task("t20", "true")
task("t21", "true")
task("t22", "true")
task("t23", "true")
task("t24", "true")
task("t25", "true")
task("t26", "true")
task("t27", "true")
task("t28", "true")
task("t29", "true")
task("t30", "true")
workflow("stress", {
    { phase = "compile", scope = "cpp" },
})
