#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"

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

TEST(OrchestratorPipelineTest, SuccessCacheSkipsCachedFilesAcrossRuns)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "lint",
    phase = "lint",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    run = function(ctx)
        local file = "src/main.cpp"
        if ctx.file_success_cached(file) then
            return 0
        end

        local job = ctx:spawn({
            cmd = "echo run >> lint_runs.txt",
        })
        local result = ctx:wait(job, { exitCode = true })
        if result.exitCode ~= 0 then
            return result.exitCode
        end

        ctx.cache_file_success(file)
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());

    ASSERT_TRUE(orchestrator.runStep("lint").hasValue());
    ASSERT_TRUE(std::filesystem::exists(Project.path() / "lint_runs.txt"));
    EXPECT_EQ(countLines(Project.path() / "lint_runs.txt"), 1U);

    ASSERT_TRUE(orchestrator.runStep("lint").hasValue());
    EXPECT_EQ(countLines(Project.path() / "lint_runs.txt"), 1U);
}

TEST(OrchestratorPipelineTest, SuccessCacheMissesAreReportedOnNextRun)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "lint",
    phase = "lint",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    run = function(ctx)
        local misses = ctx.get_cache_misses()
        local job = ctx:spawn({
            cmd = "printf '%s\n' '" .. tostring(#misses) .. "' > miss_count.txt",
        })
        local result = ctx:wait(job, { exitCode = true })
        if result.exitCode ~= 0 then
            return result.exitCode
        end

        for _, entry in ipairs(misses) do
            local append = ctx:spawn({
                cmd = "echo " .. entry .. " >> miss_count.txt",
            })
            result = ctx:wait(append, { exitCode = true })
            if result.exitCode ~= 0 then
                return result.exitCode
            end
        end

        local file = "src/main.cpp"
        if not ctx.file_success_cached(file) then
            ctx.record_file_cache_miss(file)
        end
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());

    ASSERT_TRUE(orchestrator.runStep("lint").hasValue());
    ASSERT_TRUE(orchestrator.runStep("lint").hasValue());
    EXPECT_EQ(countLines(Project.path() / "miss_count.txt"), 2U);
}

TEST(OrchestratorPipelineTest, SuccessCacheNoOpWhenCacheDisabled)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "lint",
    phase = "lint",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    run = function(ctx)
        local file = "src/main.cpp"
        ctx.cache_file_success(file)
        ctx.record_file_cache_miss(file)
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    beez::core::RunOptions options;
    options.enableCache = false;
    auto orchestrator = runtime.orchestrator(options);

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("lint").hasValue());
}
