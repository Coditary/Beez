#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"
#include "helpers/scratch_project.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <string>
#include <vector>

#ifndef BEEZ_FUZZ_CORPUS_DIR
#error "BEEZ_FUZZ_CORPUS_DIR must be defined by CMake for robustness tests"
#endif

namespace
{

std::vector<std::filesystem::path> listFuzzCorpusSeeds()
{
    const std::filesystem::path CorpusDir(BEEZ_FUZZ_CORPUS_DIR);
    if (!std::filesystem::is_directory(CorpusDir))
    {
        return {};
    }

    std::vector<std::filesystem::path> seeds;
    for (const auto& entry : std::filesystem::directory_iterator(CorpusDir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        if (entry.path().extension() == ".lua")
        {
            seeds.push_back(entry.path());
        }
    }

    return seeds;
}

void expectNoCrash(const beez::test::ProcessResult& result, const std::string& context)
{
    EXPECT_FALSE(result.terminatedBySignal) << context << ", signal=" << result.signalNumber << "\n"
                                            << result.output;
    EXPECT_TRUE(beez::test::exitedNormally(result)) << context << "\n" << result.output;
}

void expectBeezInvocationDoesNotCrash(const std::filesystem::path& projectPath,
                                      const std::initializer_list<std::string>& args,
                                      const std::string& context)
{
    expectNoCrash(beez::test::runBeez(projectPath, args), context);
}

void expectSeedInvocationDoesNotCrash(const std::filesystem::path& seed,
                                      const std::initializer_list<std::string>& args,
                                      const std::string& invocation)
{
    const beez::test::ScratchProject Project;
    Project.copyBuildLuaFrom(seed);
    expectNoCrash(beez::test::runBeez(Project.path(), args),
                  seed.filename().string() + " (" + invocation + ")");
}

std::string repeatChar(const char Character, const std::size_t Count)
{
    // NOLINTNEXTLINE(modernize-return-braced-init-list) -- braced init narrows size_t
    return std::string(Count, Character);
}

}  // namespace

TEST(SystemRobustnessTest, FuzzCorpusDirectoryExists)
{
    const std::filesystem::path CorpusDir(BEEZ_FUZZ_CORPUS_DIR);
    ASSERT_TRUE(std::filesystem::is_directory(CorpusDir))
        << "fuzz corpus directory not found: " << CorpusDir;
    ASSERT_GT(listFuzzCorpusSeeds().size(), 0U);
}

TEST(SystemRobustnessTest, FuzzCorpusSeedsDoNotCrashBeez)
{
    const auto Seeds = listFuzzCorpusSeeds();
    ASSERT_FALSE(Seeds.empty()) << "no .lua seeds in " << BEEZ_FUZZ_CORPUS_DIR;

    for (const auto& seed : Seeds)
    {
        expectSeedInvocationDoesNotCrash(seed, {"build"}, "beez build");
        expectSeedInvocationDoesNotCrash(seed, {"--list", "tasks"}, "beez --list tasks");
        expectSeedInvocationDoesNotCrash(seed, {"--show-config"}, "beez --show-config");
        expectSeedInvocationDoesNotCrash(seed, {"--list", "workflows"}, "beez --list workflows");
        expectSeedInvocationDoesNotCrash(seed, {"--list", "steps"}, "beez --list steps");
        expectSeedInvocationDoesNotCrash(seed, {"build", "--dry-run"}, "beez build --dry-run");
        expectSeedInvocationDoesNotCrash(seed, {"__no_such_target__"}, "beez unknown target");
    }
}

TEST(SystemRobustnessTest, AdversarialBuildScriptsDoNotCrashBeez)
{
    struct Scenario
    {
        const char* name;
        const char* script;
    };

    // NOLINTBEGIN(modernize-avoid-c-arrays,modernize-use-designated-initializers)
    const Scenario Scenarios[] = {
        {"empty file", ""},
        {"whitespace only", "   \n\n\t  "},
        {"comments only", "-- just a comment\n--[[ block ]]\n"},
        {"syntax error", "this is not valid lua {{{"},
        {"duplicate task names", R"(
task("build", "echo first")
task("build", "echo second")
workflow("run", {"build"})
)"},
        {"empty task table", R"(
task("empty", {})
)"},
        {"invalid step run type", R"(
step({
    name = "broken",
    phase = "generate",
    scope = "docs",
    run = 42,
})
)"},
        {"step missing name", R"(
step({
    phase = "generate",
    scope = "docs",
    run = "true",
})
)"},
        {"workflow references missing task", R"(
workflow("run", {"missing-task"})
)"},
        {"empty workflow", R"(
task("hello", "true")
workflow("run", {})
)"},
        {"nested empty tables", R"(
workflow("run", {
    {},
    { parallel = {} },
})
)"},
        {"invalid task action type", R"(
task("broken", {
    42,
})
)"},
        {"invalid beez.config type", R"(
beez.config("not-a-table")
task("hello", "true")
)"},
    };
    // NOLINTEND(modernize-avoid-c-arrays,modernize-use-designated-initializers)

    for (const auto& scenario : Scenarios)
    {
        const beez::test::ScratchProject Project;
        Project.writeBuildLua(scenario.script);

        expectBeezInvocationDoesNotCrash(
            Project.path(), {"run"}, std::string("adversarial script: ") + scenario.name);
        expectBeezInvocationDoesNotCrash(Project.path(),
                                         {"--list", "tasks"},
                                         std::string("adversarial script list: ") + scenario.name);
    }

    {
        const beez::test::ScratchProject Project;
        std::string script = R"(task("big", {)";
        for (int index = 0; index < 200; ++index)
        {
            script += "\n    \"echo line\",";
        }
        script += R"(
})
workflow("run", {"big"})
)";
        Project.writeBuildLua(script);
        expectBeezInvocationDoesNotCrash(Project.path(), {"run"}, "huge task command list");
    }
}

TEST(SystemRobustnessTest, BinaryGarbageBuildScriptDoesNotCrashBeez)
{
    const beez::test::ScratchProject Project;
    std::string bytes;
    bytes.reserve(static_cast<std::size_t>(256U) * 100U);
    for (int byte = 0; byte < 256; ++byte)
    {
        for (int repeat = 0; repeat < 100; ++repeat)
        {
            bytes.push_back(static_cast<char>(byte));
        }
    }
    Project.writeBuildLuaBytes(bytes);

    expectBeezInvocationDoesNotCrash(Project.path(), {"build"}, "binary garbage build.lua");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"--show-config"}, "binary garbage --show-config");
}

TEST(SystemRobustnessTest, VeryLongIdentifiersDoNotCrashBeez)
{
    const beez::test::ScratchProject Project;
    const std::string LongName = repeatChar('a', 8000U);
    Project.writeBuildLua("task(\"" + LongName + "\", \"true\")\nworkflow(\"run\", {\"" + LongName +
                          "\"})\n");

    expectBeezInvocationDoesNotCrash(Project.path(), {"run"}, "very long task name");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"--list", "tasks"}, "very long task name list");
}

TEST(SystemRobustnessTest, AdversarialCliArgumentsDoNotCrashBeez)
{
    const beez::test::FixtureProject Project("flag-matrix");
    const std::string LongTarget = repeatChar('z', 8000U);

    expectBeezInvocationDoesNotCrash(Project.path(), {LongTarget}, "very long target name");
    expectBeezInvocationDoesNotCrash(Project.path(), {"hello", "--dry-run"}, "hello --dry-run");
    expectBeezInvocationDoesNotCrash(Project.path(), {"fail", "--silent"}, "fail --silent");
    expectBeezInvocationDoesNotCrash(Project.path(), {"fail", "--error"}, "fail --error");
    expectBeezInvocationDoesNotCrash(Project.path(), {"echo", "--verbose"}, "echo --verbose");
    expectBeezInvocationDoesNotCrash(Project.path(), {"hello", "--no-cache"}, "hello --no-cache");
    expectBeezInvocationDoesNotCrash(Project.path(), {"hello", "-j", "16"}, "hello -j 16");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"--list", "tasks", "--list", "workflows"}, "duplicate list flags");
    expectBeezInvocationDoesNotCrash(Project.path(), {"--show-config"}, "show-config");
    expectBeezInvocationDoesNotCrash(Project.path(), {"--", "--help"}, "user option --help");
    expectBeezInvocationDoesNotCrash(Project.path(), {"--", LongTarget}, "very long user option");
}

TEST(SystemRobustnessTest, CorruptCacheDirectoryDoesNotCrashBeez)
{
    const beez::test::FixtureProject Project("flag-matrix");
    const auto CachePath = Project.path() / ".cache";
    std::filesystem::create_directories(CachePath / "entries" / "broken");

    auto writeFile =
        [&Project](const std::filesystem::path& relativePath, const std::string& content)
    {
        const auto FullPath = Project.path() / relativePath;
        if (relativePath.has_parent_path())
        {
            std::filesystem::create_directories(FullPath.parent_path());
        }
        std::ofstream stream(FullPath, std::ios::binary);
        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    };

    writeFile(".cache/entries/broken/truncated.bin", std::string(16U, '\xff'));
    writeFile(".cache/entries/broken/empty.bin", "");
    writeFile(".cache/not-a-directory", "plain text cache corruption");

    expectBeezInvocationDoesNotCrash(Project.path(), {"hello"}, "corrupt cache hello");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"hello", "--no-cache"}, "corrupt cache hello --no-cache");
    expectBeezInvocationDoesNotCrash(Project.path(), {"--clean-cache"}, "corrupt cache clean");
}

TEST(SystemRobustnessTest, WorkflowsFixtureStressPathsDoNotCrashBeez)
{
    const beez::test::FixtureProject Project("workflows");

    expectBeezInvocationDoesNotCrash(Project.path(), {"build"}, "workflows build");
    expectBeezInvocationDoesNotCrash(Project.path(), {"ci"}, "workflows ci");
    expectBeezInvocationDoesNotCrash(Project.path(), {"build", "--dry-run"}, "workflows dry-run");
    expectBeezInvocationDoesNotCrash(Project.path(), {"-s", "compile"}, "workflows step compile");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"-p", "generate:code"}, "workflows phase generate:code");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"-p", "generate:docs"}, "workflows phase generate:docs");
    expectBeezInvocationDoesNotCrash(
        Project.path(), {"__missing_workflow__"}, "workflows missing target");
}
