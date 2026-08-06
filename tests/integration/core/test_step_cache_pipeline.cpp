#include "beez/core/orchestrator.h"
#include "beez/core/phase_request.hpp"

#include "helpers/beez_runtime.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

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
