#include "beez/core/orchestrator.h"
#include "beez/core/phase_request.hpp"

#include "helpers/beez_runtime.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
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

TEST(OrchestratorPipelineTest, CachesArtifactStepsAcrossRuns)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/**/*.o" },
    run = "mkdir -p build && echo object > build/main.o",
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());

    const beez::core::PhaseRequest Request {.phase = "compile", .scopes = {"cpp"}};
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    ASSERT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.o"));

    std::filesystem::remove(Project.path() / "build" / "main.o");
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() { return 1; }\n");

    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.o"));
}

TEST(OrchestratorPipelineTest, StepCacheInvalidatesWhenBuildLuaChanges)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/out.txt" },
    run = function(ctx)
        local job = ctx:spawn({
            name = "write_out",
            cmd = "mkdir -p build && echo ok > build/out.txt && echo run >> build/runs.txt",
        })
        return ctx:wait(job)
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    std::ofstream(Project.path() / "build.lua", std::ios::app) << "-- cache bust\n";

    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(OrchestratorPipelineTest, WorkerCacheSkipsRepeatedSpawnInStep)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "work",
    phase = "work",
    scope = "demo",
    run = function(ctx)
        ctx:spawn({
            name = "worker",
            cmd = "mkdir -p build && echo run >> build/worker_runs.txt && echo ok > build/worker.out",
            inputs = { "src/main.cpp" },
            outputs = { "build/worker.out" },
        })
        return ctx:wait_all()
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("work").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "worker_runs.txt"), 1U);

    ASSERT_TRUE(orchestrator.runStep("work").hasValue());
    EXPECT_EQ(countLines(Project.path() / "build" / "worker_runs.txt"), 1U);
}
