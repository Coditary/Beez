#pragma once

#include <filesystem>

namespace beez::core
{

// Returns an absolute system temp directory. Never a relative path that would land in the
// current working directory when TMPDIR/TMP/TEMP are set to values like "tmp".
[[nodiscard]] std::filesystem::path systemTempDirectory();

}  // namespace beez::core
