task("build", "echo first > dup.out")
task("build", "echo second > dup.out")
workflow("run", {"build"})
