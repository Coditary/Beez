local M = {}

M.conanfile = "."
M.conan_binary = "conan"
M.python_binary = "python3"
M.cmake_binary = "cmake"

M.conan_output_folder = "build"
M.build_policy = "missing"

M.reports_dir = "report"
M.sbom_dir = "report/sbom"
M.graph_json = "report/sbom/conan-graph.json"
M.cyclonedx_json = "report/sbom/cyclonedx.json"
M.lockfile = "conan.lock"

M.converter_script = "plugins/coditary/conan/1.0.0/scripts/conan-graph-to-cyclonedx.py"
M.profile_script = "scripts/ci-conan-profile.sh"

M.debug_build_tree = "build/build/Debug"
M.coverage_stamp = M.debug_build_tree .. "/.beez-coverage-configured"
M.fuzzer_bin = M.debug_build_tree .. "/fuzz/fuzz_lua_dsl"

M.log_prefix_graph = "[conan-graph]"
M.log_prefix_lock = "[conan-lock]"
M.log_prefix_sbom = "[conan-sbom]"
M.log_prefix_install = "[conan-install]"
M.log_prefix_configure = "[conan-configure]"
M.log_prefix_build = "[conan-build]"

M.graph_rev = "1"
M.lock_rev = "1"
M.sbom_rev = "1"
M.install_rev = "1"
M.configure_rev = "1"
M.build_rev = "1"

M.input_patterns = {
    "conanfile.py",
    "conan/profiles/**",
}

M.configure_input_full = {
    "conanfile.py",
    "CMakeLists.txt",
    "CMakePresets.json",
    "cmake/**",
    "conan/**",
    "src/**/CMakeLists.txt",
    "tests/**/CMakeLists.txt",
}

M.configure_input_fuzzer = {
    "conanfile.py",
    "CMakeLists.txt",
    "tests/fuzz/**",
    "cmake/**",
    "conan/**",
}

M.configure_step_names = {
    "configure:setup",
    "configure:debug",
    "configure:coverage",
    "configure:sanitize",
    "configure:tsan",
    "configure:fuzzer",
}

M.build_step_names = {}

local function debug_outputs()
    return {
        M.debug_build_tree .. "/compile_commands.json",
        M.debug_build_tree .. "/build.ninja",
    }
end

M.build_profiles = {
    code = {
        scope = "code",
        build_type_env = true,
        cmake_preset_from_build_type = true,
        build_tree_from_build_type = true,
        cmake_first_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_CACHE=ON",
        },
        configure_inputs = M.configure_input_full,
        configure_outputs = function(build_tree)
            return {
                build_tree .. "/compile_commands.json",
                build_tree .. "/build.ninja",
            }
        end,
        build_inputs = function(build_tree)
            return {
                "src/**/*.cpp",
                "include/**/*.hpp",
                "tests/**/*.cpp",
                "CMakeLists.txt",
                "src/**/CMakeLists.txt",
                "tests/**/CMakeLists.txt",
                build_tree .. "/build.ninja",
            }
        end,
        build_outputs = function(build_tree)
            return {
                build_tree .. "/bin/beez",
                build_tree .. "/tests/unit/beez_tests",
                build_tree .. "/tests/integration/beez_integration_tests",
                build_tree .. "/tests/system/beez_system_tests",
                build_tree .. "/tests/performance/beez_perf_tests",
            }
        end,
        configure_description = function(build_type)
            return "Conan install + CMake configure (" .. build_type .. ")"
        end,
        build_description = "CMake build (app + all test binaries)",
    },

    debug = {
        scope = "debug",
        build_type = "Debug",
        cmake_preset = "conan-debug",
        build_tree = M.debug_build_tree,
        cmake_first_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_CACHE=ON",
        },
        configure_inputs = M.configure_input_full,
        configure_outputs = debug_outputs(),
        build_inputs = {
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
            M.debug_build_tree .. "/build.ninja",
        },
        build_outputs = { M.debug_build_tree .. "/bin/beez" },
        configure_description = "Conan install + CMake configure (Debug)",
        build_description = "CMake Debug build",
    },

    coverage = {
        scope = "coverage",
        build_type = "Debug",
        cmake_preset = "conan-debug",
        build_tree = M.debug_build_tree,
        cmake_first_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_CACHE=ON",
            "-DBUILD_COVERAGE=ON",
            "-DBUILD_FUZZER=OFF",
            "-DENABLE_ASAN=OFF",
            "-DENABLE_UBSAN=OFF",
        },
        post_configure = function(root)
            return "grep -qE 'BUILD_COVERAGE:(BOOL|UNINITIALIZED)=ON' " ..
                root .. "/" .. M.debug_build_tree .. "/CMakeCache.txt " ..
                "&& touch " .. root .. "/" .. M.coverage_stamp
        end,
        configure_inputs = {
            "conanfile.py",
            "CMakeLists.txt",
            "cmake/**",
            "src/**/CMakeLists.txt",
            "tests/**/CMakeLists.txt",
        },
        configure_outputs = {
            M.debug_build_tree .. "/compile_commands.json",
            M.debug_build_tree .. "/build.ninja",
            M.coverage_stamp,
        },
        build_inputs = {
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
            M.debug_build_tree .. "/build.ninja",
            M.coverage_stamp,
        },
        build_outputs = { M.debug_build_tree .. "/tests/unit/beez_tests" },
        configure_description = "CMake configure with coverage instrumentation",
        build_description = "Build Debug with coverage flags",
    },

    sanitize = {
        scope = "sanitize",
        build_type = "Debug",
        cmake_preset = "conan-debug",
        build_tree = M.debug_build_tree,
        cmake_first_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_CACHE=ON",
        },
        cmake_second_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_COVERAGE=OFF",
            "-DBUILD_FUZZER=OFF",
            "-DENABLE_ASAN=ON",
            "-DENABLE_UBSAN=ON",
        },
        post_configure = function(root)
            return "rm -f " .. root .. "/" .. M.coverage_stamp
        end,
        configure_inputs = M.configure_input_full,
        configure_outputs = debug_outputs(),
        build_inputs = {
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
            M.debug_build_tree .. "/build.ninja",
        },
        build_outputs = { M.debug_build_tree .. "/tests/unit/beez_tests" },
        configure_description = "CMake configure with ASan/UBSan",
        build_description = "Build Debug with sanitizers",
    },

    tsan = {
        scope = "tsan",
        build_type = "Debug",
        cmake_preset = "conan-debug",
        build_tree = M.debug_build_tree,
        cmake_first_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_CACHE=ON",
        },
        cmake_second_args = {
            "-DBUILD_TESTING=ON",
            "-DBUILD_COVERAGE=OFF",
            "-DBUILD_FUZZER=OFF",
            "-DENABLE_ASAN=OFF",
            "-DENABLE_UBSAN=OFF",
            "-DENABLE_TSAN=ON",
        },
        post_configure = function(root)
            return "rm -f " .. root .. "/" .. M.coverage_stamp
        end,
        configure_inputs = M.configure_input_full,
        configure_outputs = debug_outputs(),
        build_inputs = {
            "src/**/*.cpp",
            "include/**/*.hpp",
            "tests/**/*.cpp",
            M.debug_build_tree .. "/build.ninja",
        },
        build_outputs = { M.debug_build_tree .. "/tests/unit/beez_tests" },
        configure_description = "CMake configure with ThreadSanitizer",
        build_description = "Build Debug with ThreadSanitizer",
    },

    fuzzer = {
        scope = "fuzz",
        build_type = "Debug",
        cmake_preset = "conan-debug",
        build_tree = M.debug_build_tree,
        cmake_first_args = {
            "-DBUILD_TESTING=OFF",
            "-DBUILD_CACHE=ON",
        },
        cmake_second_args = {
            "-DBUILD_TESTING=OFF",
            "-DBUILD_COVERAGE=OFF",
            "-DBUILD_FUZZER=ON",
            "-DENABLE_ASAN=OFF",
            "-DENABLE_UBSAN=OFF",
        },
        configure_inputs = M.configure_input_fuzzer,
        configure_outputs = { M.debug_build_tree .. "/build.ninja" },
        build_inputs = {
            "tests/fuzz/**",
            M.debug_build_tree .. "/build.ninja",
        },
        build_outputs = { M.fuzzer_bin },
        build_target = "fuzz_lua_dsl",
        configure_description = "CMake configure for fuzzer target",
        build_description = "Build fuzz_lua_dsl",
    },
}

M.configure_step_profiles = {
    ["configure:setup"] = "code",
    ["configure:debug"] = "debug",
    ["configure:coverage"] = "coverage",
    ["configure:sanitize"] = "sanitize",
    ["configure:tsan"] = "tsan",
    ["configure:fuzzer"] = "fuzzer",
}

M.build_step_profiles = {}

return M
