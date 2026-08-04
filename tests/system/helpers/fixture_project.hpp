#pragma once

#include <atomic>
#include <filesystem>
#include <stdexcept>
#include <string>

#ifndef BEEZ_SYSTEM_FIXTURES_DIR
#error "BEEZ_SYSTEM_FIXTURES_DIR must be defined by CMake for system tests"
#endif

namespace beez::test
{

class FixtureProject
{
  public:
    explicit FixtureProject(const std::string& fixtureName)
    {
        const std::filesystem::path Source =
            std::filesystem::path(BEEZ_SYSTEM_FIXTURES_DIR) / fixtureName;
        if (!std::filesystem::exists(Source))
        {
            throw std::runtime_error("system fixture not found: " + Source.string());
        }

        const auto Counter = idCounter.fetch_add(1);
        path_ = std::filesystem::temp_directory_path() /
                ("beez_system_" + fixtureName + "_" + std::to_string(Counter));
        std::filesystem::create_directories(path_);
        std::filesystem::copy(Source,
                              path_,
                              std::filesystem::copy_options::recursive |
                                  std::filesystem::copy_options::overwrite_existing);
    }

    ~FixtureProject()
    {
        std::filesystem::remove_all(path_);
    }

    FixtureProject(const FixtureProject&) = delete;
    FixtureProject& operator=(const FixtureProject&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const
    {
        return path_;
    }

    [[nodiscard]] bool hasFile(const std::filesystem::path& relativePath) const
    {
        return std::filesystem::exists(path_ / relativePath);
    }

  private:
    std::filesystem::path path_;
    static inline std::atomic<int> idCounter {0};
};

}  // namespace beez::test
