#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>

namespace beez::test
{

struct ProcessResult
{
    int exitCode = -1;
    std::string output;
};

[[nodiscard]] ProcessResult runBeez(const std::filesystem::path& workingDir,
                                    const std::initializer_list<std::string>& args);

}  // namespace beez::test
