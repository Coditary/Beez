#pragma once

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace beez::test
{

// Header-only mirror of beez::core::systemTempDirectory() for test targets that do not link
// beez_core (for example system tests).
inline std::filesystem::path testTempDirectory()
{
    for (const char* variable : {"TMPDIR", "TMP", "TEMP"})
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
        if (const char* value = std::getenv(variable); value != nullptr && value[0] != '\0')
        {
            const std::filesystem::path Configured(value);
            if (!Configured.empty() && Configured.is_absolute())
            {
                return Configured;
            }
        }
    }

    std::error_code errorCode;
    const std::filesystem::path FromStd = std::filesystem::temp_directory_path(errorCode);
    if (!errorCode && !FromStd.empty() && FromStd.is_absolute())
    {
        // NOLINTNEXTLINE(performance-no-automatic-move) -- returning local path by value
        return FromStd;
    }

    return {"/tmp"};
}

}  // namespace beez::test
