#include "beez/plugin/lua/api/archive/detail/archive_ops.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace beez::plugin::lua::archive_detail
{

namespace
{

constexpr std::size_t ReadBufferSize = 64 * 1024;
constexpr std::size_t MaxTextReadBytes = 16 * 1024 * 1024;
constexpr int ExtractFlags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                             ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;

struct ArchiveReadHandle
{
    archive* reader = nullptr;

    ArchiveReadHandle()
    {
        reader = archive_read_new();
        if (reader == nullptr)
        {
            throw std::runtime_error("failed to allocate archive reader");
        }

        archive_read_support_filter_all(reader);
        archive_read_support_format_all(reader);
    }

    ~ArchiveReadHandle()
    {
        if (reader != nullptr)
        {
            archive_read_close(reader);
            archive_read_free(reader);
        }
    }

    ArchiveReadHandle(const ArchiveReadHandle&) = delete;
    ArchiveReadHandle& operator=(const ArchiveReadHandle&) = delete;

    ArchiveReadHandle(ArchiveReadHandle&& other) noexcept : reader(other.reader)
    {
        other.reader = nullptr;
    }

    ArchiveReadHandle& operator=(ArchiveReadHandle&& other) noexcept
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
};

struct ArchiveWriteHandle
{
    archive* writer = nullptr;

    explicit ArchiveWriteHandle(archive* handle) : writer(handle)
    {
        if (writer == nullptr)
        {
            throw std::runtime_error("failed to allocate archive writer");
        }
    }

    ~ArchiveWriteHandle()
    {
        if (writer != nullptr)
        {
            archive_write_close(writer);
            archive_write_free(writer);
        }
    }

    ArchiveWriteHandle(const ArchiveWriteHandle&) = delete;
    ArchiveWriteHandle& operator=(const ArchiveWriteHandle&) = delete;
};

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

[[nodiscard]] std::string archiveError(archive* handle)
{
    return archive_error_string(handle);
}

[[nodiscard]] void throwOnArchiveError(archive* handle, const int result, const std::string& context)
{
    if (result < ARCHIVE_OK)
    {
        throw std::runtime_error(context + ": " + archiveError(handle));
    }
}

[[nodiscard]] std::string normalizeEntryPath(std::string path)
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

[[nodiscard]] bool entryPathsMatch(std::string_view left, std::string_view right)
{
    return normalizeEntryPath(std::string(left)) == normalizeEntryPath(std::string(right));
}

struct ArchiveFormatSpec
{
    int format = ARCHIVE_FORMAT_ZIP;
    int filter = ARCHIVE_FILTER_NONE;
};

[[nodiscard]] std::optional<ArchiveFormatSpec> formatFromName(std::string_view name)
{
    const std::string Lower = toLower(std::string(name));
    if (Lower == "zip")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_ZIP};
    }
    if (Lower == "tar")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR};
    }
    if (Lower == "tar.gz" || Lower == "tgz")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_GZIP};
    }
    if (Lower == "tar.bz2" || Lower == "tbz2")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_BZIP2};
    }
    if (Lower == "tar.xz" || Lower == "txz")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_XZ};
    }

    return std::nullopt;
}

[[nodiscard]] ArchiveFormatSpec detectFormat(const std::filesystem::path& archivePath,
                                             const std::optional<std::string>& formatOverride)
{
    if (formatOverride.has_value())
    {
        const auto Parsed = formatFromName(*formatOverride);
        if (!Parsed.has_value())
        {
            throw std::invalid_argument("unsupported archive format: " + *formatOverride);
        }
        return *Parsed;
    }

    const std::string Extension = toLower(archivePath.extension().string());
    if (Extension == ".zip")
    {
        return {.format = ARCHIVE_FORMAT_ZIP};
    }
    if (Extension == ".tar")
    {
        return {.format = ARCHIVE_FORMAT_TAR};
    }

    const std::string Filename = toLower(archivePath.filename().string());
    if (Filename.ends_with(".tar.gz") || Filename.ends_with(".tgz"))
    {
        return {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_GZIP};
    }
    if (Filename.ends_with(".tar.bz2") || Filename.ends_with(".tbz2"))
    {
        return {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_BZIP2};
    }
    if (Filename.ends_with(".tar.xz") || Filename.ends_with(".txz"))
    {
        return {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_XZ};
    }

    throw std::invalid_argument("could not detect archive format from path: " + archivePath.string());
}

[[nodiscard]] ArchiveReadHandle openArchiveReader(const std::filesystem::path& archivePath)
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

[[nodiscard]] ArchiveEntryInfo entryInfo(archive_entry* entry)
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

void addFileToArchive(archive* writer,
                      const std::filesystem::path& sourcePath,
                      const std::string& entryName)
{
    archive_entry* entry = archive_entry_new();
    if (entry == nullptr)
    {
        throw std::runtime_error("failed to allocate archive entry");
    }

    archive_entry_set_pathname(entry, entryName.c_str());
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);

    std::ifstream stream(sourcePath, std::ios::binary);
    if (!stream)
    {
        archive_entry_free(entry);
        throw std::runtime_error("failed to open source file: " + sourcePath.string());
    }

    stream.seekg(0, std::ios::end);
    const auto Size = static_cast<la_int64_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    archive_entry_set_size(entry, Size);

    throwOnArchiveError(writer, archive_write_header(writer, entry), "failed to write archive header");

    std::array<char, ReadBufferSize> buffer {};
    while (stream)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize Read = stream.gcount();
        if (Read <= 0)
        {
            break;
        }

        throwOnArchiveError(writer,
                            static_cast<int>(archive_write_data(writer, buffer.data(),
                                                                 static_cast<std::size_t>(Read))),
                            "failed to write archive data");
    }

    archive_entry_free(entry);
}

void addDirectoryToArchive(archive* writer,
                           const std::filesystem::path& rootPath,
                           const std::filesystem::path& currentPath,
                           const std::string& entryPrefix)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             currentPath, std::filesystem::directory_options::skip_permission_denied))
    {
        const std::filesystem::path Relative = std::filesystem::relative(entry.path(), rootPath);
        const std::string EntryName =
            entryPrefix.empty() ? Relative.generic_string() : entryPrefix + "/" + Relative.generic_string();

        if (entry.is_directory())
        {
            archive_entry* archiveEntry = archive_entry_new();
            archive_entry_set_pathname(archiveEntry, (EntryName + "/").c_str());
            archive_entry_set_filetype(archiveEntry, AE_IFDIR);
            archive_entry_set_perm(archiveEntry, 0755);
            archive_entry_set_size(archiveEntry, 0);
            throwOnArchiveError(writer,
                                archive_write_header(writer, archiveEntry),
                                "failed to write directory header");
            archive_entry_free(archiveEntry);
            continue;
        }

        if (entry.is_regular_file())
        {
            addFileToArchive(writer, entry.path(), EntryName);
        }
    }
}

}  // namespace

std::vector<ArchiveEntryInfo> listEntries(const std::filesystem::path& archivePath)
{
    ArchiveReadHandle handle = openArchiveReader(archivePath);
    std::vector<ArchiveEntryInfo> entries;
    archive_entry* entry = nullptr;

    while (true)
    {
        const int Result = archive_read_next_header(handle.reader, &entry);
        if (Result == ARCHIVE_EOF)
        {
            break;
        }
        throwOnArchiveError(handle.reader, Result, "failed to read archive header");

        ArchiveEntryInfo info = entryInfo(entry);
        if (!info.path.empty())
        {
            entries.push_back(std::move(info));
        }
    }

    return entries;
}

std::size_t extractAll(const std::filesystem::path& archivePath,
                       const std::filesystem::path& destinationDir)
{
    std::filesystem::create_directories(destinationDir);
    ArchiveReadHandle handle = openArchiveReader(archivePath);

    const std::filesystem::path PreviousCwd = std::filesystem::current_path();
    std::filesystem::current_path(destinationDir);

    std::size_t extractedCount = 0;
    archive_entry* entry = nullptr;
    try
    {
        while (true)
        {
            const int Result = archive_read_next_header(handle.reader, &entry);
            if (Result == ARCHIVE_EOF)
            {
                break;
            }
            throwOnArchiveError(handle.reader, Result, "failed to read archive header");

            const std::string RelativePath = normalizeEntryPath(archive_entry_pathname(entry));
            archive_entry_set_pathname(entry, RelativePath.c_str());
            throwOnArchiveError(handle.reader,
                                archive_read_extract(handle.reader, entry, ExtractFlags),
                                "failed to extract archive entry");
            ++extractedCount;
        }
    }
    catch (...)
    {
        std::filesystem::current_path(PreviousCwd);
        throw;
    }

    std::filesystem::current_path(PreviousCwd);
    return extractedCount;
}

void extractFile(const std::filesystem::path& archivePath,
                 const std::string& entryPath,
                 const std::filesystem::path& destinationPath)
{
    ArchiveReadHandle handle = openArchiveReader(archivePath);
    archive_entry* entry = nullptr;
    bool found = false;

    while (true)
    {
        const int Result = archive_read_next_header(handle.reader, &entry);
        if (Result == ARCHIVE_EOF)
        {
            break;
        }
        throwOnArchiveError(handle.reader, Result, "failed to read archive header");

        if (!entryPathsMatch(archive_entry_pathname(entry), entryPath))
        {
            archive_read_data_skip(handle.reader);
            continue;
        }

        if (archive_entry_filetype(entry) == AE_IFDIR)
        {
            throw std::runtime_error("archive entry is a directory: " + entryPath);
        }

        std::filesystem::create_directories(destinationPath.parent_path());
        std::ofstream output(destinationPath, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error("failed to open destination file: " + destinationPath.string());
        }

        std::array<char, ReadBufferSize> buffer {};
        while (true)
        {
            const la_ssize_t Read = archive_read_data(handle.reader, buffer.data(), buffer.size());
            if (Read == 0)
            {
                break;
            }
            if (Read < 0)
            {
                throw std::runtime_error("failed to read archive entry: " + archiveError(handle.reader));
            }
            output.write(buffer.data(), Read);
        }

        found = true;
        break;
    }

    if (!found)
    {
        throw std::runtime_error("archive entry not found: " + entryPath);
    }
}

std::string readText(const std::filesystem::path& archivePath, const std::string& entryPath)
{
    ArchiveReadHandle handle = openArchiveReader(archivePath);
    archive_entry* entry = nullptr;

    while (true)
    {
        const int Result = archive_read_next_header(handle.reader, &entry);
        if (Result == ARCHIVE_EOF)
        {
            break;
        }
        throwOnArchiveError(handle.reader, Result, "failed to read archive header");

        if (!entryPathsMatch(archive_entry_pathname(entry), entryPath))
        {
            archive_read_data_skip(handle.reader);
            continue;
        }

        if (archive_entry_filetype(entry) == AE_IFDIR)
        {
            throw std::runtime_error("archive entry is a directory: " + entryPath);
        }

        const auto EntrySize = static_cast<std::uint64_t>(archive_entry_size(entry));
        if (EntrySize > MaxTextReadBytes)
        {
            throw std::runtime_error("archive entry is too large to read as text: " + entryPath);
        }

        std::string content;
        content.reserve(static_cast<std::size_t>(EntrySize));
        std::array<char, ReadBufferSize> buffer {};
        while (true)
        {
            const la_ssize_t Read = archive_read_data(handle.reader, buffer.data(), buffer.size());
            if (Read == 0)
            {
                break;
            }
            if (Read < 0)
            {
                throw std::runtime_error("failed to read archive entry: " + archiveError(handle.reader));
            }
            content.append(buffer.data(), static_cast<std::size_t>(Read));
        }

        return content;
    }

    throw std::runtime_error("archive entry not found: " + entryPath);
}

void compress(const std::filesystem::path& sourcePath,
              const std::filesystem::path& archivePath,
              const std::optional<std::string>& formatOverride)
{
    if (!std::filesystem::exists(sourcePath))
    {
        throw std::invalid_argument("source does not exist: " + sourcePath.string());
    }

    const ArchiveFormatSpec Format = detectFormat(archivePath, formatOverride);
    std::filesystem::create_directories(archivePath.parent_path());

    archive* writer = archive_write_new();
    ArchiveWriteHandle handle(writer);
    throwOnArchiveError(writer, archive_write_set_format(writer, Format.format), "failed to set archive format");
    if (Format.filter != ARCHIVE_FILTER_NONE)
    {
        throwOnArchiveError(writer, archive_write_add_filter(writer, Format.filter), "failed to set archive filter");
    }

    throwOnArchiveError(writer,
                        archive_write_open_filename(writer, archivePath.string().c_str()),
                        "failed to open archive for writing");

    if (std::filesystem::is_directory(sourcePath))
    {
        addDirectoryToArchive(writer, sourcePath, sourcePath, "");
    }
    else
    {
        addFileToArchive(writer, sourcePath, sourcePath.filename().string());
    }
}

}  // namespace beez::plugin::lua::archive_detail
