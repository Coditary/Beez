#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>

namespace beez::test
{

class ScratchProject
{
  public:
    ScratchProject()
    {
        const auto Counter = idCounter.fetch_add(1);
        path_ = std::filesystem::temp_directory_path() /
                ("beez_system_scratch_" + std::to_string(Counter));
        std::filesystem::create_directories(path_);
    }

    ~ScratchProject()
    {
        std::filesystem::remove_all(path_);
    }

    ScratchProject(const ScratchProject&) = delete;
    ScratchProject& operator=(const ScratchProject&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const
    {
        return path_;
    }

    void writeBuildLua(const std::string& content) const
    {
        std::ofstream stream(path_ / "build.lua");
        if (!stream)
        {
            throw std::runtime_error("failed to write build.lua in scratch project");
        }
        stream << content;
    }

    void copyBuildLuaFrom(const std::filesystem::path& source) const
    {
        if (!std::filesystem::exists(source))
        {
            throw std::runtime_error("fuzz corpus seed not found: " + source.string());
        }
        std::filesystem::copy_file(
            source, path_ / "build.lua", std::filesystem::copy_options::overwrite_existing);
    }

    void writeBuildLuaBytes(const std::string& bytes) const
    {
        std::ofstream stream(path_ / "build.lua", std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("failed to write binary build.lua in scratch project");
        }
        stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    void writeFile(const std::filesystem::path& relativePath, const std::string& content) const
    {
        const auto FullPath = path_ / relativePath;
        if (relativePath.has_parent_path())
        {
            std::filesystem::create_directories(FullPath.parent_path());
        }
        std::ofstream stream(FullPath, std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("failed to write file in scratch project: " +
                                     relativePath.string());
        }
        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

  private:
    std::filesystem::path path_;
    static inline std::atomic<int> idCounter {0};
};

}  // namespace beez::test
