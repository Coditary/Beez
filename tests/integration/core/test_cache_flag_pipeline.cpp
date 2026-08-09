#include "beez/core/run_options.hpp"

#include "helpers/beez_runtime.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{

void writeProjectFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

std::size_t countLines(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    std::size_t count = 0;
    std::string line;
    while (std::getline(stream, line))
    {
        ++count;
    }
    return count;
}

}  // namespace

TEST(CacheFlagPipelineTest, EnabledCacheSkipsRepeatedArtifactStep)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/runs.txt" },
    run = "mkdir -p build && echo run >> build/runs.txt",
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);
}

TEST(CacheFlagPipelineTest, DisabledCacheReexecutesArtifactStep)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/runs.txt" },
    run = "mkdir -p build && echo run >> build/runs.txt",
})
)");

    beez::core::RunOptions options;
    options.enableCache = false;
    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator(options);

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(CacheFlagPipelineTest, CustomCacheRootStoresStepCacheEntries)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/main.o" },
    run = "mkdir -p build && echo object > build/main.o",
})
)");

    beez::core::RunOptions options;
    options.enableCache = true;
    options.cache.root = Project.path() / "manual-cache";

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator(options);

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());

    EXPECT_TRUE(std::filesystem::exists(Project.path() / "manual-cache" / "entries"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.o"));
}
