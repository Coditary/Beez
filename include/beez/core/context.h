#pragma once

#include <filesystem>

namespace beez::core
{

class Context
{
  public:
    explicit Context(std::filesystem::path projectRoot = std::filesystem::current_path());

    [[nodiscard]] const std::filesystem::path& projectRoot() const
    {
        return projectRoot_;
    }

    [[nodiscard]] std::filesystem::path buildScriptPath() const;

  private:
    std::filesystem::path projectRoot_;
};

}  // namespace beez::core
