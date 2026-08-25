#include "beez/cli/session.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/core/util/temp_directory.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace
{

class ScopedEnv
{
  public:
    ScopedEnv(const char* name, const char* value)
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
        : name_(name), hadValue_(std::getenv(name) != nullptr)
    {
        if (hadValue_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
            saved_ = std::getenv(name);
        }

        if (value[0] == '\0')
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c,misc-include-cleaner)
            unsetenv(name);
            unset_ = true;
        }
        else
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c,misc-include-cleaner)
            setenv(name, value, 1);
            unset_ = false;
        }
    }

    ~ScopedEnv()
    {
        if (unset_)
        {
            if (hadValue_)
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,misc-include-cleaner)
                setenv(name_.c_str(), saved_.c_str(), 1);
            }
            return;
        }

        if (hadValue_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,misc-include-cleaner)
            setenv(name_.c_str(), saved_.c_str(), 1);
        }
        else
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,misc-include-cleaner)
            unsetenv(name_.c_str());
        }
    }

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

  private:
    std::string name_;
    bool hadValue_ = false;
    bool unset_ = false;
    std::string saved_;
};

class ScopedTempTree
{
  public:
    explicit ScopedTempTree(std::filesystem::path path) : path_(std::move(path)) {}

    ~ScopedTempTree()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(path_, errorCode);
    }

    ScopedTempTree(const ScopedTempTree&) = delete;
    ScopedTempTree& operator=(const ScopedTempTree&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const
    {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

class ParametersTestEnv
{
  public:
    ParametersTestEnv()
        : root_(beez::core::systemTempDirectory() / "beez_integration_parameters_env"),
          xdgEnv_("XDG_CONFIG_HOME", (root_ / "xdg").c_str()), cleanup_(root_)
    {
        std::filesystem::create_directories(configDir());
        std::filesystem::create_directories(profilesDir());
    }

    [[nodiscard]] std::filesystem::path configDir() const
    {
        return root_ / "xdg" / "beez";
    }
    [[nodiscard]] std::filesystem::path profilesDir() const
    {
        return configDir() / "profiles";
    }

    void writeGlobalConfig(const std::string& content) const
    {
        std::ofstream stream(configDir() / "config.lua");
        stream << content;
    }

  private:
    std::filesystem::path root_;
    ScopedEnv xdgEnv_;
    ScopedTempTree cleanup_;
};

[[nodiscard]] beez::cli::LoadedProject makeLoadedProject(const std::filesystem::path& projectRoot)
{
    beez::cli::LoadedProject project;
    project.context = beez::core::Context(projectRoot);
    return project;
}

void writeMetaJson(const beez::test::TempProject& project, const std::string& content)
{
    std::ofstream stream(project.path() / "meta.json");
    stream << content;
}

[[nodiscard]] std::optional<std::string> shellCommandOf(const beez::cli::LoadedProject& project,
                                                        const std::string& taskName)
{
    const auto Task = project.registry.findTask(taskName);
    if (!Task.has_value() || Task->actions.empty())
    {
        return std::nullopt;
    }

    const auto* shellAction = std::get_if<beez::core::TaskShellAction>(&Task->actions.front());
    if (shellAction == nullptr)
    {
        return std::nullopt;
    }

    return shellAction->command;
}

constexpr const char* MetaJson = R"({
    "properties": {"greeting": "hello", "server": {"host": "localhost", "port": 8080}},
    "profiles": {"dev": {"greeting": "hi dev", "server": {"host": "127.0.0.1"}}}
})";

}  // namespace

TEST(SessionParametersIntegrationTest, DefinesReachBuildScriptWithoutParametersCall)
{
    const ParametersTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("show", "echo flag=" .. tostring(beez.var.extra.flag))
)");

    auto project = makeLoadedProject(Project.path());
    project.context.setParameterDefines({"extra.flag=on"});

    ASSERT_FALSE(loadGlobalSettings(project).has_value());
    ASSERT_FALSE(loadBuildScript(project, false).has_value());

    const auto Command = shellCommandOf(project, "show");
    ASSERT_TRUE(Command.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(*Command, "echo flag=on");
}

TEST(SessionParametersIntegrationTest, ProfileSelectionDrivesParametersLookup)
{
    const ParametersTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    writeMetaJson(Project, MetaJson);
    Project.writeBuildLua(R"(
parameters("meta.json")
task("show", "echo greeting=" .. beez.var.greeting .. " host=" .. beez.var.server.host)
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(loadBuildScript(project, false).has_value());

    const auto Command = shellCommandOf(project, "show");
    ASSERT_TRUE(Command.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(*Command, "echo greeting=hi dev host=127.0.0.1");
}

TEST(SessionParametersIntegrationTest, DefinesOverrideProfileAndProperties)
{
    const ParametersTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    writeMetaJson(Project, MetaJson);
    Project.writeBuildLua(R"(
parameters("meta.json")
task("show", "echo greeting=" .. beez.var.greeting)
)");

    auto project = makeLoadedProject(Project.path());
    project.context.setParameterDefines({"greeting=bye"});

    ASSERT_FALSE(loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(loadBuildScript(project, false).has_value());

    const auto Command = shellCommandOf(project, "show");
    ASSERT_TRUE(Command.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(*Command, "echo greeting=bye");
}

TEST(SessionParametersIntegrationTest, UnknownProfileFallsBackToProperties)
{
    const ParametersTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    writeMetaJson(Project, MetaJson);
    Project.writeBuildLua(R"(
parameters("meta.json")
task("show", "echo greeting=" .. beez.var.greeting .. " port=" .. tostring(beez.var.server.port))
)");

    auto project = makeLoadedProject(Project.path());

    // Missing Lua profile file must not abort; the name stays active.
    ASSERT_FALSE(loadGlobalSettings(project, "staging").has_value());
    ASSERT_FALSE(loadBuildScript(project, false).has_value());

    const auto Command = shellCommandOf(project, "show");
    ASSERT_TRUE(Command.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(*Command, "echo greeting=hello port=8080");
}

TEST(SessionParametersIntegrationTest, MultipleParameterFilesMergeInOrder)
{
    const ParametersTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    {
        std::ofstream first(Project.path() / "a.json");
        first << R"({"properties": {"one": 1}})";
    }
    {
        std::ofstream second(Project.path() / "b.json");
        second << R"({"properties": {"one": 11, "two": 2}})";
    }
    Project.writeBuildLua(R"(
parameters("a.json", "b.json")
task("show", "echo one=" .. tostring(beez.var.one) .. " two=" .. tostring(beez.var.two))
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(loadGlobalSettings(project).has_value());
    ASSERT_FALSE(loadBuildScript(project, false).has_value());

    const auto Command = shellCommandOf(project, "show");
    ASSERT_TRUE(Command.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(*Command, "echo one=11 two=2");
}

TEST(SessionParametersIntegrationTest, BrokenParameterFileAbortsLoadWithError)
{
    const ParametersTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    {
        std::ofstream broken(Project.path() / "broken.json");
        broken << R"({"properties": )";
    }
    Project.writeBuildLua(R"(
parameters("broken.json")
task("show", "echo never")
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(loadGlobalSettings(project).has_value());
    EXPECT_TRUE(loadBuildScript(project, false).has_value()) << "invalid JSON should abort load";
    EXPECT_FALSE(project.registry.findTask("show").has_value());
}
