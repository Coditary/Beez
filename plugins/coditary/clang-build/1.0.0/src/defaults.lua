local M = {}

M.debug_build_tree = "build/build/Debug"
M.fuzzer_bin = M.debug_build_tree .. "/fuzz/fuzz_lua_dsl"
M.coverage_stamp = M.debug_build_tree .. "/.beez-coverage-configured"

M.index_script = "plugins/coditary/clang-build/1.0.0/scripts/compdb_index.py"
M.python_binary = "python3"

M.log_prefix_compile = "[clang-compile]"
M.log_prefix_link = "[clang-link]"

M.compile_rev = "1"
M.link_rev = "1"

M.link_order = {
    "prebyte_core",
    "tempify_core",
    "beez_logging",
    "beez_core",
    "beez_plugin_host",
    "beez_plugin_shell",
    "beez_plugin_lua",
    "beez_cli",
    "beez",
    "beez_tests",
    "beez_integration_tests",
    "beez_system_tests",
    "beez_perf_tests",
    "fuzz_lua_dsl",
}

M.build_profiles = {
    code = {
        scope = "app",
        build_type_env = true,
        build_tree_from_build_type = true,
        compile_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "cmake/**",
            "conan/**",
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
        },
        compile_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/.beez-clang-index.lua",
                build_tree .. "/.beez-clang-scripts/**",
            }
        end,
        link_outputs = function(build_tree)
            return {
                build_tree .. "/bin/beez",
                build_tree .. "/tests/unit/beez_tests",
                build_tree .. "/tests/integration/beez_integration_tests",
                build_tree .. "/tests/system/beez_system_tests",
                build_tree .. "/tests/performance/beez_perf_tests",
            }
        end,
        compile_description = "Clang compile (Release/Debug via BUILD_TYPE)",
        link_description = "Clang link (app + all test binaries)",
    },

    debug = {
        scope = "debug",
        build_type = "Debug",
        build_tree = M.debug_build_tree,
        compile_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "cmake/**",
            "src/**/*.cpp",
            "include/**/*.hpp",
        },
        compile_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/.beez-clang-index.lua",
                build_tree .. "/.beez-clang-scripts/**",
            }
        end,
        link_outputs = { M.debug_build_tree .. "/bin/beez" },
        compile_description = "Clang compile (Debug)",
        link_description = "Clang link (Debug app)",
    },

    coverage = {
        scope = "coverage",
        build_type = "Debug",
        build_tree = M.debug_build_tree,
        compile_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "cmake/**",
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
        },
        compile_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/.beez-clang-index.lua",
                M.coverage_stamp,
            }
        end,
        link_outputs = { M.debug_build_tree .. "/tests/unit/beez_tests" },
        compile_description = "Clang compile with coverage instrumentation",
        link_description = "Clang link coverage test binary",
    },

    sanitize = {
        scope = "sanitize",
        build_type = "Debug",
        build_tree = M.debug_build_tree,
        compile_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "cmake/**",
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
        },
        compile_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/.beez-clang-index.lua",
                build_tree .. "/.beez-clang-scripts/**",
            }
        end,
        link_outputs = { M.debug_build_tree .. "/tests/unit/beez_tests" },
        compile_description = "Clang compile with ASan/UBSan",
        link_description = "Clang link sanitizer test binary",
    },

    tsan = {
        scope = "tsan",
        build_type = "Debug",
        build_tree = M.debug_build_tree,
        compile_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "cmake/**",
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
        },
        compile_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/.beez-clang-index.lua",
                build_tree .. "/.beez-clang-scripts/**",
            }
        end,
        link_outputs = { M.debug_build_tree .. "/tests/unit/beez_tests" },
        compile_description = "Clang compile with ThreadSanitizer",
        link_description = "Clang link TSan test binary",
    },

    fuzzer = {
        scope = "fuzz",
        build_type = "Debug",
        build_tree = M.debug_build_tree,
        compile_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "tests/fuzz/**",
            "cmake/**",
        },
        compile_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/.beez-clang-index.lua",
                build_tree .. "/.beez-clang-scripts/**",
            }
        end,
        link_outputs = { M.fuzzer_bin },
        compile_description = "Clang compile fuzz_lua_dsl dependencies",
        link_description = "Clang link fuzz_lua_dsl",
    },
}

M.compile_step_profiles = {
    ["compile:code"] = "code",
    ["compile:debug"] = "debug",
    ["compile:coverage"] = "coverage",
    ["compile:sanitize"] = "sanitize",
    ["compile:tsan"] = "tsan",
    ["compile:fuzzer"] = "fuzzer",
}

M.link_step_profiles = {
    ["link:code"] = "code",
    ["link:debug"] = "debug",
    ["link:coverage"] = "coverage",
    ["link:sanitize"] = "sanitize",
    ["link:tsan"] = "tsan",
    ["link:fuzzer"] = "fuzzer",
}

return M
