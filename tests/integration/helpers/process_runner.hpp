#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>

namespace beez::test
{

struct ProcessResult
{
    int exitCode = -1;
    bool terminatedBySignal = false;
    int signalNumber = 0;
    std::string output;
};

[[nodiscard]] ProcessResult runBeez(const std::filesystem::path& workingDir,
                                    const std::initializer_list<std::string>& args);

[[nodiscard]] inline bool exitedNormally(const ProcessResult& result)
{
    return !result.terminatedBySignal && result.exitCode >= 0;
}

[[nodiscard]] inline bool outputContains(const ProcessResult& result, const std::string& needle)
{
    return result.output.find(needle) != std::string::npos;
}

[[nodiscard]] inline bool outputEmpty(const ProcessResult& result)
{
    return result.output.empty();
}

[[nodiscard]] ProcessResult runShellScript(const std::filesystem::path& scriptPath,
                                           const std::initializer_list<std::string>& envVars = {});

}  // namespace beez::test
