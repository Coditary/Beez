#pragma once

#include <archive.h>
#include <archive_entry.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace beez::plugin::lua::archive_detail
{

constexpr int ExtractFlags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
                             ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                             ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;

struct ArchiveEntryInfo
{
    std::string path;
    std::uint64_t size = 0;
    bool isDirectory = false;
};

struct ArchiveReadHandle
{
    archive* reader = nullptr;

    ArchiveReadHandle();
    ~ArchiveReadHandle();

    ArchiveReadHandle(const ArchiveReadHandle&) = delete;
    ArchiveReadHandle& operator=(const ArchiveReadHandle&) = delete;
    ArchiveReadHandle(ArchiveReadHandle&& other) noexcept;
    ArchiveReadHandle& operator=(ArchiveReadHandle&& other) noexcept;
};

[[nodiscard]] std::string normalizeEntryPath(std::string path);
[[nodiscard]] bool entryPathsMatch(std::string_view left, std::string_view right);
[[nodiscard]] std::string archiveError(archive* handle);
void throwOnArchiveError(archive* handle, int result, const std::string& context);
[[nodiscard]] ArchiveReadHandle openArchiveReader(const std::filesystem::path& archivePath);
[[nodiscard]] ArchiveEntryInfo entryInfo(archive_entry* entry);

}  // namespace beez::plugin::lua::archive_detail
