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

[[nodiscard]] inline bool outputContains(const ProcessResult& result, const std::string& needle)
{
    return result.output.find(needle) != std::string::npos;
}

}  // namespace beez::test
