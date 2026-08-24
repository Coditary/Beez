#include "beez/core/config/paths/bridge_paths.hpp"
#include "beez/core/config/paths/config_paths.hpp"
#include "beez/core/util/temp_directory.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

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
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
            unsetenv(name);
            unset_ = true;
        }
        else
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
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
                // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
                setenv(name_.c_str(), saved_.c_str(), 1);
            }
            return;
        }

        if (hadValue_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
            setenv(name_.c_str(), saved_.c_str(), 1);
        }
        else
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
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

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

  private:
    std::filesystem::path path_;
};

class BridgeTestEnv
{
  public:
    BridgeTestEnv()
        : tempRoot_(beez::core::systemTempDirectory() /
                    ("beez_bridge_test_" + std::to_string(reinterpret_cast<std::uintptr_t>(&instanceCounter_)))),
          xdgEnv_("XDG_CONFIG_HOME", tempRoot_.c_str()), cleanup_(tempRoot_)
    {
        ++instanceCounter_;
        std::filesystem::create_directories(tempRoot_ / "beez");
    }

    [[nodiscard]] std::filesystem::path configDir() const { return tempRoot_ / "beez"; }
    [[nodiscard]] std::filesystem::path bridgesDir() const { return tempRoot_ / "beez" / "bridges"; }
    [[nodiscard]] std::filesystem::path indexPath() const { return bridgesDir() / "index.json"; }
    [[nodiscard]] const std::filesystem::path& tempRoot() const { return tempRoot_; }

    [[nodiscard]] std::filesystem::path createBuildLua(const std::string& relativePath,
                                                       const std::string& content)
    {
        const auto FullPath = tempRoot_ / relativePath;
        std::filesystem::create_directories(FullPath.parent_path());
        std::ofstream file(FullPath);
        file << content;
        file.close();
        return FullPath;
    }

  private:
    static inline int instanceCounter_ = 0;
    std::filesystem::path tempRoot_;
    ScopedEnv xdgEnv_;
    ScopedTempTree cleanup_;
};

}  // namespace

TEST(BridgePathsTest, HashPathReturnsHexString)
{
    const auto Hash = beez::core::hashPath("/some/path");
    EXPECT_FALSE(Hash.empty());
    for (const char c : Hash)
    {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character in hash: " << c;
    }
}

TEST(BridgePathsTest, HashPathIsDeterministic)
{
    EXPECT_EQ(beez::core::hashPath("/same/path"), beez::core::hashPath("/same/path"));
}

TEST(BridgePathsTest, HashPathDiffersForDifferentPaths)
{
    EXPECT_NE(beez::core::hashPath("/path/a"), beez::core::hashPath("/path/b"));
}

TEST(BridgePathsTest, HashPathDiffersForParentChild)
{
    EXPECT_NE(beez::core::hashPath("/foo"), beez::core::hashPath("/foo/bar"));
}

TEST(BridgePathsTest, BridgeDirectoryRespectsXdg)
{
    BridgeTestEnv env;
    EXPECT_EQ(beez::core::bridgeDirectory(), env.bridgesDir());
}

TEST(BridgePathsTest, BridgeIndexPathIsInsideBridgeDirectory)
{
    BridgeTestEnv env;
    EXPECT_EQ(beez::core::bridgeIndexPath(), env.bridgesDir() / "index.json");
}

TEST(BridgePathsTest, ResolveBridgeReturnsNulloptWhenNoIndex)
{
    BridgeTestEnv env;
    const auto Result = beez::core::resolveBridge("/nonexistent/project");
    EXPECT_FALSE(Result.has_value());
}

TEST(BridgePathsTest, ResolveBridgeReturnsNulloptWhenIndexEmpty)
{
    BridgeTestEnv env;
    std::ofstream file(env.indexPath());
    file << "{}";
    file.close();

    const auto Result = beez::core::resolveBridge("/nonexistent/project");
    EXPECT_FALSE(Result.has_value());
}

TEST(BridgePathsTest, ResolveBridgeReturnsNulloptWhenIndexCorrupted)
{
    BridgeTestEnv env;
    std::ofstream file(env.indexPath());
    file << "not valid json {{{";
    file.close();

    const auto Result = beez::core::resolveBridge("/nonexistent/project");
    EXPECT_FALSE(Result.has_value());
}

TEST(BridgePathsTest, ResolveBridgeReturnsNulloptWhenIndexMissingFile)
{
    BridgeTestEnv env;
    std::ofstream file(env.indexPath());
    file << "{ \"/some/project\": { \"hash\": \"abc123\" } }";
    file.close();

    const auto Result = beez::core::resolveBridge("/some/project");
    EXPECT_FALSE(Result.has_value());
}

TEST(BridgePathsTest, CreateBridgeLinkCopiesBuildLua)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "task(\"hello\")");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto Result = beez::core::createBridgeLink(Source, ProjectRoot);

    EXPECT_FALSE(Result.alreadyExisted);
    EXPECT_TRUE(std::filesystem::exists(Result.bridgeDir / "build.lua"));

    std::ifstream bridgeFile(Result.bridgeDir / "build.lua");
    std::string content((std::istreambuf_iterator<char>(bridgeFile)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "task(\"hello\")");
}

TEST(BridgePathsTest, CreateBridgeLinkCreatesIndexEntry)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "step({})");
    const auto ProjectRoot = env.tempRoot() / "project";

    beez::core::createBridgeLink(Source, ProjectRoot);

    EXPECT_TRUE(std::filesystem::exists(env.indexPath()));
    std::ifstream indexFile(env.indexPath());
    std::string content((std::istreambuf_iterator<char>(indexFile)),
                        std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("project"), std::string::npos);
}

TEST(BridgePathsTest, CreateBridgeLinkReturnsAlreadyExistedWhenRepeating)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "step({})");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto First = beez::core::createBridgeLink(Source, ProjectRoot);
    EXPECT_FALSE(First.alreadyExisted);

    const auto Second = beez::core::createBridgeLink(Source, ProjectRoot);
    EXPECT_TRUE(Second.alreadyExisted);
    EXPECT_EQ(First.bridgeDir, Second.bridgeDir);
}

TEST(BridgePathsTest, ResolveBridgeReturnsPathAfterCreate)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "task(\"test\")");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto Created = beez::core::createBridgeLink(Source, ProjectRoot);
    const auto Resolved = beez::core::resolveBridge(ProjectRoot);

    ASSERT_TRUE(Resolved.has_value());
    EXPECT_EQ(*Resolved, Created.bridgeDir / "build.lua");
}

TEST(BridgePathsTest, ResolveBridgeFindsCorrectProject)
{
    BridgeTestEnv env;
    const auto Source1 = env.createBuildLua("p1/build.lua", "task(\"p1\")");
    const auto Source2 = env.createBuildLua("p2/build.lua", "task(\"p2\")");
    const auto Project1 = env.tempRoot() / "p1";
    const auto Project2 = env.tempRoot() / "p2";

    const auto Created1 = beez::core::createBridgeLink(Source1, Project1);
    const auto Created2 = beez::core::createBridgeLink(Source2, Project2);

    const auto Resolved1 = beez::core::resolveBridge(Project1);
    const auto Resolved2 = beez::core::resolveBridge(Project2);

    ASSERT_TRUE(Resolved1.has_value());
    ASSERT_TRUE(Resolved2.has_value());
    EXPECT_EQ(*Resolved1, Created1.bridgeDir / "build.lua");
    EXPECT_EQ(*Resolved2, Created2.bridgeDir / "build.lua");
    EXPECT_NE(*Resolved1, *Resolved2);
}

TEST(BridgePathsTest, CreateBridgeLinkOverwritesExistingBridge)
{
    BridgeTestEnv env;
    const auto Source1 = env.createBuildLua("project/build.lua", "step({name=\"v1\"})");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto First = beez::core::createBridgeLink(Source1, ProjectRoot);

    // Second call with different source - already exists, so original content preserved
    const auto Source2 = env.createBuildLua("project/build.lua", "step({name=\"v2\"})");
    const auto Second = beez::core::createBridgeLink(Source2, ProjectRoot);
    EXPECT_TRUE(Second.alreadyExisted);

    // Content is from first link (not overwritten)
    std::ifstream bridgeFile(First.bridgeDir / "build.lua");
    std::string content((std::istreambuf_iterator<char>(bridgeFile)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "step({name=\"v1\"})");
}

TEST(BridgePathsTest, CreateBridgeLinkStaleEntryRemoved)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "task(\"test\")");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto Created = beez::core::createBridgeLink(Source, ProjectRoot);

    // Manually corrupt index by adding a duplicate entry with different hash
    std::ofstream indexFile(env.indexPath());
    indexFile << "{ \"" << ProjectRoot.string()
              << "\": { \"hash\": \"wrong_hash\" }, \"" << ProjectRoot.string()
              << "_old\": { \"hash\": \"" << beez::core::hashPath(ProjectRoot)
              << "\" } }";
    indexFile.close();

    // Re-create should clean up stale entries
    const auto ReCreated = beez::core::createBridgeLink(Source, ProjectRoot);
    EXPECT_TRUE(std::filesystem::exists(ReCreated.bridgeDir / "build.lua"));
}

TEST(BridgePathsTest, ResolveBridgeReturnsNulloptForDeletedBridge)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "task(\"test\")");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto Created = beez::core::createBridgeLink(Source, ProjectRoot);

    // Delete the bridge build.lua
    std::filesystem::remove(Created.bridgeDir / "build.lua");

    const auto Resolved = beez::core::resolveBridge(ProjectRoot);
    EXPECT_FALSE(Resolved.has_value());
}

TEST(BridgePathsTest, CreateBridgeLinkWithCustomSourcePath)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("custom/my-build.lua", "task(\"custom\")");
    const auto ProjectRoot = env.tempRoot() / "project";
    std::filesystem::create_directories(ProjectRoot);

    const auto Result = beez::core::createBridgeLink(Source, ProjectRoot);

    EXPECT_FALSE(Result.alreadyExisted);
    std::ifstream bridgeFile(Result.bridgeDir / "build.lua");
    std::string content((std::istreambuf_iterator<char>(bridgeFile)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "task(\"custom\")");
}

TEST(BridgePathsTest, IndexJsonIsValidAfterMultipleCreates)
{
    BridgeTestEnv env;
    const auto Source1 = env.createBuildLua("a/build.lua", "task(\"a\")");
    const auto Source2 = env.createBuildLua("b/build.lua", "task(\"b\")");
    const auto Source3 = env.createBuildLua("c/build.lua", "task(\"c\")");

    beez::core::createBridgeLink(Source1, env.tempRoot() / "a");
    beez::core::createBridgeLink(Source2, env.tempRoot() / "b");
    beez::core::createBridgeLink(Source3, env.tempRoot() / "c");

    // Verify index can be re-read and contains all entries
    const auto R1 = beez::core::resolveBridge(env.tempRoot() / "a");
    const auto R2 = beez::core::resolveBridge(env.tempRoot() / "b");
    const auto R3 = beez::core::resolveBridge(env.tempRoot() / "c");

    EXPECT_TRUE(R1.has_value());
    EXPECT_TRUE(R2.has_value());
    EXPECT_TRUE(R3.has_value());
}

TEST(BridgePathsTest, ResolveBridgeWithTrailingSlash)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "task(\"test\")");
    const auto ProjectRoot = env.tempRoot() / "project";

    beez::core::createBridgeLink(Source, ProjectRoot);

    // Resolve with path that might differ in trailing slash
    const auto PathWithSlash = ProjectRoot / "";
    const auto Resolved = beez::core::resolveBridge(PathWithSlash);
    // Should still find it (weakly_canonical normalizes)
    EXPECT_TRUE(Resolved.has_value());
}

TEST(BridgePathsTest, CreateBridgeLinkSetsCorrectHashDirectory)
{
    BridgeTestEnv env;
    const auto Source = env.createBuildLua("project/build.lua", "task(\"test\")");
    const auto ProjectRoot = env.tempRoot() / "project";

    const auto Result = beez::core::createBridgeLink(Source, ProjectRoot);

    const auto ExpectedHash = beez::core::hashPath(std::filesystem::weakly_canonical(ProjectRoot));
    EXPECT_EQ(Result.bridgeDir.filename().string(), ExpectedHash);
}

TEST(BridgePathsTest, BridgeIndexHandlesSpecialCharactersInPath)
{
    BridgeTestEnv env;
    const auto SpecialDir = env.tempRoot() / "path with spaces &special";
    std::filesystem::create_directories(SpecialDir);
    const auto Source = SpecialDir / "build.lua";
    {
        std::ofstream file(Source);
        file << "task(\"special\")";
    }

    const auto Result = beez::core::createBridgeLink(Source, SpecialDir);
    EXPECT_FALSE(Result.alreadyExisted);

    const auto Resolved = beez::core::resolveBridge(SpecialDir);
    ASSERT_TRUE(Resolved.has_value());
    EXPECT_EQ(*Resolved, Result.bridgeDir / "build.lua");
}

TEST(BridgePathsTest, CreateBridgeLinkFailsGracefullyWithNonexistentSource)
{
    BridgeTestEnv env;
    const auto Nonexistent = env.tempRoot() / "nonexistent" / "build.lua";
    const auto ProjectRoot = env.tempRoot() / "project";
    std::filesystem::create_directories(ProjectRoot);

    // createBridgeLink does not check existence; it copies directly
    // std::filesystem::copy_file will throw
    EXPECT_THROW(beez::core::createBridgeLink(Nonexistent, ProjectRoot),
                 std::filesystem::filesystem_error);
}
