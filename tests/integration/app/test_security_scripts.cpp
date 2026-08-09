#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

TEST(SecurityScriptsTest, AdversarialShellChecksPass)
{
    const auto ScriptPath = std::filesystem::path(__FILE__).parent_path().parent_path() /
                            "scripts" / "test_security_scripts.sh";
    ASSERT_TRUE(std::filesystem::exists(ScriptPath));

    const beez::test::ProcessResult Result = beez::test::runShellScript(ScriptPath);
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
}
