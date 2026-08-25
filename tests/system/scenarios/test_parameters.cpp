#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>

namespace
{

[[nodiscard]] std::string runAndReadResult(const beez::test::FixtureProject& project,
                                           const std::initializer_list<std::string>& args)
{
    const beez::test::ProcessResult Result = beez::test::runBeez(project.path(), args);
    EXPECT_EQ(Result.exitCode, 0);

    std::ifstream stream(project.path() / "result.txt");
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

}  // namespace

TEST(SystemParametersTest, LoadsPropertiesWithoutProfile)
{
    const beez::test::FixtureProject Project("parameters-basic");

    EXPECT_EQ(runAndReadResult(Project, {"show"}),
              "greeting=hello host=localhost port=8080 flag=nil\n");
}

TEST(SystemParametersTest, ProfileFlagSelectsJsonProfileWithDeepMerge)
{
    const beez::test::FixtureProject Project("parameters-basic");

    // Profile overrides greeting and server.host; properties port survives.
    EXPECT_EQ(runAndReadResult(Project, {"--profile=dev", "show"}),
              "greeting=hi dev host=127.0.0.1 port=8080 flag=nil\n");
}

TEST(SystemParametersTest, DefinesOverrideProfileAndProperties)
{
    const beez::test::FixtureProject Project("parameters-basic");

    // CLI values always win over profile and properties; dot paths create
    // nested tables on the fly.
    EXPECT_EQ(
        runAndReadResult(Project, {"--profile=dev", "-Dgreeting=bye", "-Dextra.flag=on", "show"}),
        "greeting=bye host=127.0.0.1 port=8080 flag=on\n");
}

TEST(SystemParametersTest, DefineAloneOverridesProperties)
{
    const beez::test::FixtureProject Project("parameters-basic");

    EXPECT_EQ(runAndReadResult(Project, {"-Dserver.host=example.org", "show"}),
              "greeting=hello host=example.org port=8080 flag=nil\n");
}

TEST(SystemParametersTest, UnknownProfileFallsBackToPropertiesWithoutError)
{
    const beez::test::FixtureProject Project("parameters-basic");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile=staging", "show"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "Build finished"));

    std::ifstream stream(Project.path() / "result.txt");
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    EXPECT_EQ(buffer.str(), "greeting=hello host=localhost port=8080 flag=nil\n");
}

TEST(SystemParametersTest, BrokenParameterJsonAbortsRun)
{
    const beez::test::FixtureProject Project("parameters-basic");
    Project.writeFile("broken.json", R"({"properties": )");
    Project.writeFile("build.lua",
                      "parameters(\"broken.json\")\n"
                      "task(\"show\", \"echo never > result.txt\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"show"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_FALSE(Project.hasFile("result.txt"));
}
