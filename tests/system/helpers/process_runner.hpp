#pragma once

#include <filesystem>
#include <initializer_list>
#include <sstream>
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

[[nodiscard]] inline std::string stripProfilingLines(const std::string& output)
{
    std::ostringstream filtered;
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.rfind("profiling:", 0) != 0)
        {
            filtered << line << '\n';
        }
    }

    return filtered.str();
}

[[nodiscard]] inline bool outputContainsTaskName(const ProcessResult& result,
                                                 const std::string& taskName)
{
    const std::string filtered = stripProfilingLines(result.output);
    std::istringstream stream(filtered);
    std::string line;
    bool afterSeparator = false;
    while (std::getline(stream, line))
    {
        if (line == "----")
        {
            afterSeparator = true;
            continue;
        }

        if (afterSeparator && line == taskName)
        {
            return true;
        }
    }

    return false;
}

}  // namespace beez::test
