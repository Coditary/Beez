#pragma once

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
        path_ = std::filesystem::temp_directory_path() / ("beez_test_" + std::to_string(Counter));
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

  private:
    std::filesystem::path path_;
    static inline std::atomic<int> idCounter {0};
};

}  // namespace beez::test
