#include "beez/plugin/lua/api/archive/detail/archive_common.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::archive_detail
{

ArchiveReadHandle::ArchiveReadHandle()
{
    reader = archive_read_new();
    if (reader == nullptr)
    {
        throw std::runtime_error("failed to allocate archive reader");
    }

    archive_read_support_filter_all(reader);
    archive_read_support_format_all(reader);
}

ArchiveReadHandle::~ArchiveReadHandle()
{
    if (reader != nullptr)
    {
        archive_read_close(reader);
        archive_read_free(reader);
    }
}

ArchiveReadHandle::ArchiveReadHandle(ArchiveReadHandle&& other) noexcept : reader(other.reader)
{
    other.reader = nullptr;
}

ArchiveReadHandle& ArchiveReadHandle::operator=(ArchiveReadHandle&& other) noexcept
{
    if (this != &other)
    {
        if (reader != nullptr)
        {
            archive_read_close(reader);
            archive_read_free(reader);
        }
        reader = other.reader;
        other.reader = nullptr;
    }
    return *this;
}

std::string normalizeEntryPath(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    while (path.starts_with("./"))
    {
        path.erase(0, 2);
    }
    while (!path.empty() && path.front() == '/')
    {
        path.erase(path.begin());
    }
    return path;
}

bool entryPathsMatch(const std::string_view left, const std::string_view right)
{
    return normalizeEntryPath(std::string(left)) == normalizeEntryPath(std::string(right));
}

std::string archiveError(archive* handle)
{
    return archive_error_string(handle);
}

void throwOnArchiveError(archive* handle, const int result, const std::string& context)
{
    if (result < ARCHIVE_OK)
    {
        throw std::runtime_error(context + ": " + archiveError(handle));
    }
}

ArchiveReadHandle openArchiveReader(const std::filesystem::path& archivePath)
{
    if (!std::filesystem::exists(archivePath))
    {
        throw std::invalid_argument("archive does not exist: " + archivePath.string());
    }

    ArchiveReadHandle handle;
    throwOnArchiveError(handle.reader,
                        archive_read_open_filename(handle.reader, archivePath.string().c_str(), 10240),
                        "failed to open archive");
    return handle;
}

ArchiveEntryInfo entryInfo(archive_entry* entry)
{
    ArchiveEntryInfo info;
    if (const char* pathname = archive_entry_pathname(entry); pathname != nullptr)
    {
        info.path = normalizeEntryPath(pathname);
    }
    info.size = static_cast<std::uint64_t>(archive_entry_size(entry));
    info.isDirectory = archive_entry_filetype(entry) == AE_IFDIR;
    return info;
}

}  // namespace beez::plugin::lua::archive_detail
