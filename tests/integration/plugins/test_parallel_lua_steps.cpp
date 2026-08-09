#include "helpers/process_runner.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(ParallelLuaStepsIntegrationTest, QualityWorkflowWithMultipleLuaStepsAndThreadsSucceeds)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("quality", {
    { phase = "qa", scope = "code" },
})

step({
    name = "qa:alpha",
    phase = "qa",
    scope = "code",
    run = function(ctx)
        return 0
    end,
})

step({
    name = "qa:beta",
    phase = "qa",
    scope = "code",
    run = function(ctx)
        return 0
    end,
})

step({
    name = "qa:gamma",
    phase = "qa",
    scope = "code",
    run = function(ctx)
        return 0
    end,
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"quality", "--threads", "4"});

    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_NE(Result.output.find("quality"), std::string::npos);
}
