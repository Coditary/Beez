task("hello", "echo hello > hello.out")
task("fail", "false")
task("clean", "rm -f hello.out")
