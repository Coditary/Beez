#include "beez/core/util/temp_directory.hpp"

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace beez::core
{

namespace
{

[[nodiscard]] bool isAbsoluteTempPath(const std::filesystem::path& path)
{
    return !path.empty() && path.is_absolute();
}

[[nodiscard]] std::filesystem::path tempFromEnvironment(const char* variable)
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (const char* value = std::getenv(variable); value != nullptr && value[0] != '\0')
    {
        const std::filesystem::path Configured(value);
        if (isAbsoluteTempPath(Configured))
        {
            return Configured;
        }
    }

    return {};
}

[[nodiscard]] std::filesystem::path tempFromStdLibrary()
{
    std::error_code errorCode;
    const std::filesystem::path FromStd = std::filesystem::temp_directory_path(errorCode);
    if (!errorCode && isAbsoluteTempPath(FromStd))
    {
        // NOLINTNEXTLINE(performance-no-automatic-move) -- returning local path by value
        return FromStd;
    }

    return {};
}

}  // namespace

std::filesystem::path systemTempDirectory()
{
    for (const char* variable : {"TMPDIR", "TMP", "TEMP"})
    {
        const auto FromEnv = tempFromEnvironment(variable);
        if (!FromEnv.empty())
        {
            return FromEnv;
        }
    }

    const auto FromStd = tempFromStdLibrary();
    if (!FromStd.empty())
    {
        // NOLINTNEXTLINE(performance-no-automatic-move) -- returning local path by value
        return FromStd;
    }

    return {"/tmp"};
}

}  // namespace beez::core
