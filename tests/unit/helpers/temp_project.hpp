#pragma once

#include "beez/core/util/temp_directory.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

namespace beez::test
{

class TempProject
{
  public:
    TempProject()
    {
        const auto Counter = idCounter.fetch_add(1);
        path_ = beez::core::systemTempDirectory() / ("beez_test_" + std::to_string(Counter));
        std::filesystem::create_directories(path_);
    }

    ~TempProject()
    {
        std::filesystem::remove_all(path_);
    }

    TempProject(const TempProject&) = delete;
    TempProject& operator=(const TempProject&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const
    {
        return path_;
    }

    void writeBuildLua(const std::string& content) const
    {
        std::ofstream stream(path_ / "build.lua");
        stream << content;
    }

    void writeDotEnv(const std::string& content) const
    {
        std::ofstream stream(path_ / ".env");
        stream << content;
    }

    void writePlugin(const std::string& organization,
                     const std::string& name,
                     const std::string& version,
                     const std::string& content) const
    {
        const auto PluginPath =
            path_ / ".cache" / "beez" / "plugins" / organization / name / version /
            "beez_plugin.lua";
        std::filesystem::create_directories(PluginPath.parent_path());
        std::ofstream stream(PluginPath);
        stream << content;
    }

  private:
    std::filesystem::path path_;
    static inline std::atomic<int> idCounter {0};
};

}  // namespace beez::test
