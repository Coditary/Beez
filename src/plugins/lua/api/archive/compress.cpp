#include "beez/plugin/lua/api/archive/compress.hpp"

#include "beez/plugin/lua/api/archive/detail/archive_common.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-implicit-widening-of-multiplication-result,bugprone-easily-swappable-parameters,cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-special-member-functions,misc-non-private-member-variables-in-classes,readability-identifier-naming)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

constexpr std::size_t ReadBufferSize = 64U * 1024U;

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

struct ArchiveFormatSpec
{
    int format = ARCHIVE_FORMAT_ZIP;
    int filter = ARCHIVE_FILTER_NONE;
};

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });
    return value;
}

[[nodiscard]] std::optional<std::string>
parseFormatOption(const sol::optional<sol::table>& optionsTable)
{
    if (!optionsTable.has_value())
    {
        return std::nullopt;
    }

    const sol::object formatValue = optionsTable->get<sol::object>("format");
    if (!formatValue.valid() || formatValue.is<sol::lua_nil_t>())
    {
        return std::nullopt;
    }

    if (!formatValue.is<std::string>())
    {
        throw std::runtime_error("archive compress option 'format' must be a string");
    }

    return formatValue.as<std::string>();
}

[[nodiscard]] std::optional<ArchiveFormatSpec> formatFromName(std::string_view name)
{
    const std::string lower = toLower(std::string(name));
    if (lower == "zip")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_ZIP};
    }
    if (lower == "tar")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR};
    }
    if (lower == "tar.gz" || lower == "tgz")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_GZIP};
    }
    if (lower == "tar.bz2" || lower == "tbz2")
    {
        return ArchiveFormatSpec {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_BZIP2};
    }
    if (lower == "tar.xz" || lower == "txz")
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
        const auto parsed = formatFromName(*formatOverride);
        if (!parsed.has_value())
        {
            throw std::invalid_argument("unsupported archive format: " + *formatOverride);
        }
        return *parsed;
    }

    const std::string extension = toLower(archivePath.extension().string());
    if (extension == ".zip")
    {
        return {.format = ARCHIVE_FORMAT_ZIP};
    }
    if (extension == ".tar")
    {
        return {.format = ARCHIVE_FORMAT_TAR};
    }

    const std::string filename = toLower(archivePath.filename().string());
    if (filename.ends_with(".tar.gz") || filename.ends_with(".tgz"))
    {
        return {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_GZIP};
    }
    if (filename.ends_with(".tar.bz2") || filename.ends_with(".tbz2"))
    {
        return {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_BZIP2};
    }
    if (filename.ends_with(".tar.xz") || filename.ends_with(".txz"))
    {
        return {.format = ARCHIVE_FORMAT_TAR, .filter = ARCHIVE_FILTER_XZ};
    }

    throw std::invalid_argument("could not detect archive format from path: " +
                                archivePath.string());
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
    const auto size = static_cast<la_int64_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    archive_entry_set_size(entry, size);

    archive_detail::throwOnArchiveError(
        writer, archive_write_header(writer, entry), "failed to write archive header");

    std::array<char, ReadBufferSize> buffer {};
    while (stream)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize read = stream.gcount();
        if (read <= 0)
        {
            break;
        }

        archive_detail::throwOnArchiveError(
            writer,
            static_cast<int>(
                archive_write_data(writer, buffer.data(), static_cast<std::size_t>(read))),
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
        const std::filesystem::path relative = std::filesystem::relative(entry.path(), rootPath);
        const std::string entryName = entryPrefix.empty()
                                          ? relative.generic_string()
                                          : entryPrefix + "/" + relative.generic_string();

        if (entry.is_directory())
        {
            archive_entry* archiveEntry = archive_entry_new();
            archive_entry_set_pathname(archiveEntry, (entryName + "/").c_str());
            archive_entry_set_filetype(archiveEntry, AE_IFDIR);
            archive_entry_set_perm(archiveEntry, 0755);
            archive_entry_set_size(archiveEntry, 0);
            archive_detail::throwOnArchiveError(writer,
                                                archive_write_header(writer, archiveEntry),
                                                "failed to write directory header");
            archive_entry_free(archiveEntry);
            continue;
        }

        if (entry.is_regular_file())
        {
            addFileToArchive(writer, entry.path(), entryName);
        }
    }
}

void compress(const std::filesystem::path& sourcePath,
              const std::filesystem::path& archivePath,
              const std::optional<std::string>& formatOverride)
{
    if (!std::filesystem::exists(sourcePath))
    {
        throw std::invalid_argument("source does not exist: " + sourcePath.string());
    }

    const ArchiveFormatSpec format = detectFormat(archivePath, formatOverride);
    std::filesystem::create_directories(archivePath.parent_path());

    archive* writer = archive_write_new();
    const ArchiveWriteHandle handle(writer);
    archive_detail::throwOnArchiveError(
        writer, archive_write_set_format(writer, format.format), "failed to set archive format");
    if (format.filter != ARCHIVE_FILTER_NONE)
    {
        archive_detail::throwOnArchiveError(writer,
                                            archive_write_add_filter(writer, format.filter),
                                            "failed to set archive filter");
    }

    archive_detail::throwOnArchiveError(
        writer,
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

}  // namespace

void bindArchiveCompress(sol::table& archiveTable,
                         const std::shared_ptr<sol::state>& luaState,
                         const core::Context& context)
{
    archiveTable["compress"] =
        [luaState, &context](const std::string& sourcePath,
                             const std::string& archivePath,
                             const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        const std::filesystem::path resolvedSource =
            api_detail::resolvePath(context.projectRoot(), sourcePath);
        const std::filesystem::path resolvedArchive =
            api_detail::resolvePath(context.projectRoot(), archivePath);
        compress(resolvedSource, resolvedArchive, parseFormatOption(optionsTable));

        sol::table result = luaState->create_table();
        result["path"] = resolvedArchive.generic_string();
        if (std::filesystem::exists(resolvedArchive))
        {
            result["bytes"] =
                static_cast<std::uint64_t>(std::filesystem::file_size(resolvedArchive));
        }
        return result;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-implicit-widening-of-multiplication-result,bugprone-easily-swappable-parameters,cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-special-member-functions,misc-non-private-member-variables-in-classes,readability-identifier-naming)
