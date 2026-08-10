#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace beez::plugin::lua::archive_detail
{

struct ArchiveEntryInfo
{
    std::string path;
    std::uint64_t size = 0;
    bool isDirectory = false;
};

[[nodiscard]] std::vector<ArchiveEntryInfo> listEntries(const std::filesystem::path& archivePath);

[[nodiscard]] std::size_t extractAll(const std::filesystem::path& archivePath,
                                     const std::filesystem::path& destinationDir);

void extractFile(const std::filesystem::path& archivePath,
                 const std::string& entryPath,
                 const std::filesystem::path& destinationPath);

[[nodiscard]] std::string readText(const std::filesystem::path& archivePath,
                                   const std::string& entryPath);

void compress(const std::filesystem::path& sourcePath,
              const std::filesystem::path& archivePath,
              const std::optional<std::string>& formatOverride = std::nullopt);

}  // namespace beez::plugin::lua::archive_detail
