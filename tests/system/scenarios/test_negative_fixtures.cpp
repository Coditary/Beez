#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{

void expectLoadFailure(const beez::test::FixtureProject& project, const std::string& target)
{
    const beez::test::ProcessResult Result = beez::test::runBeez(project.path(), {target});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_NE(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "failed to load build script")) << Result.output;
}

}  // namespace

TEST(SystemNegativeFixtureTest, EmptyTaskTableFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-empty-task");
    expectLoadFailure(Project, "empty");
}

TEST(SystemNegativeFixtureTest, InvalidStepRunTypeFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-invalid-step-run");
    expectLoadFailure(Project, "broken");
}

TEST(SystemNegativeFixtureTest, StepMissingNameFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-missing-step-name");
    expectLoadFailure(Project, "run");
}

TEST(SystemNegativeFixtureTest, InvalidTaskActionTypeFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-invalid-task-action");
    expectLoadFailure(Project, "broken");
}

TEST(SystemNegativeFixtureTest, InvalidBeezConfigFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-invalid-config");
    expectLoadFailure(Project, "hello");
}

TEST(SystemNegativeFixtureTest, StepMissingRunFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-step-missing-run");
    expectLoadFailure(Project, "run");
}

TEST(SystemNegativeFixtureTest, DuplicateTaskNamesFailToLoad)
{
    const beez::test::FixtureProject Project("negative-duplicate-tasks");
    expectLoadFailure(Project, "run");
}

TEST(SystemNegativeFixtureTest, WorkflowMissingTaskReferenceFailsToLoad)
{
    const beez::test::FixtureProject Project("negative-workflow-missing-ref");
    expectLoadFailure(Project, "run");
}

TEST(SystemNegativeFixtureTest, EmptyWorkflowCompletesWithoutRunningSteps)
{
    const beez::test::FixtureProject Project("negative-empty-workflow");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"run"});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
}
