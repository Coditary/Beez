from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import can_run
import os


class BeezConan(ConanFile):
    name = "beez"
    version = "1.0.2"
    package_type = "application"
    license = "Apache-2.0"
    url = "https://github.com/Leodoras/Beez"

    settings = "os", "compiler", "build_type", "arch"
    options = {"build_testing": [True, False]}
    default_options = {"build_testing": True}

    exports_sources = "CMakeLists.txt", "src/*", "include/*", "tests/*", "build.lua"

    generators = "CMakeDeps", "CMakeToolchain"

    def configure(self):
        self.options["hwloc"].shared = True

    def requirements(self):
        if self.options.build_testing:
            self.requires("gtest/1.14.0")
        self.requires("lua/5.4.6")
        self.requires("cli11/2.6.2")
        self.requires("spdlog/1.15.3")
        self.requires("onetbb/2022.0.0")
        self.requires("zlib/1.3.1")
        self.requires("yyjson/0.12.0")
        self.requires("rapidyaml/0.15.2")
        self.requires("rapidxml/1.13")
        self.requires("fast-cpp-csv-parser/cci.20240102")
        self.requires("tomlplusplus/3.4.0")
        self.requires("libcurl/8.12.1")
        self.requires("libarchive/3.7.7")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.bindir, "beez_tests")
            self.run(cmd, env="conanrun")
