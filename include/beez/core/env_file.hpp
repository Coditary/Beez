#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace beez::core
{

class EnvFile
{
  public:
    explicit EnvFile(std::filesystem::path path);

    [[nodiscard]] std::optional<std::string> lookup(const std::string& key) const;

  private:
    void ensureLoaded() const;

    std::filesystem::path path_;
    mutable bool loaded_ = false;
    mutable std::unordered_map<std::string, std::string> values_;
};

[[nodiscard]] std::optional<std::string> readProcessEnvironment(const std::string& key);

}  // namespace beez::core
