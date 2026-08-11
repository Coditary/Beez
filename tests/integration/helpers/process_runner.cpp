// NOLINTBEGIN(misc-include-cleaner)
#include "helpers/process_runner.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <sys/wait.h>

#ifndef BEEZ_EXECUTABLE
#error "BEEZ_EXECUTABLE must be defined by CMake for integration tests"
#endif

namespace beez::test
{

namespace
{

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (const char Character : value)
    {
        if (Character == '\'')
        {
            quoted += "'\\''";
        }
        else
        {
            quoted += Character;
        }
    }
    quoted += '\'';
    return quoted;
}

ProcessResult finalizeProcessStatus(const int WaitStatus, std::string output)
{
    if (WaitStatus == -1)
    {
        return {.exitCode = -1, .output = std::move(output)};
    }

    if (WIFSIGNALED(WaitStatus))
    {
        return {.exitCode = -1,
                .terminatedBySignal = true,
                .signalNumber = WTERMSIG(WaitStatus),
                .output = std::move(output)};
    }

    int exitCode = -1;
    if (WIFEXITED(WaitStatus))
    {
        exitCode = WEXITSTATUS(WaitStatus);
    }

    return {.exitCode = exitCode, .output = std::move(output)};
}

}  // namespace

ProcessResult runBeez(const std::filesystem::path& workingDir,
                      const std::initializer_list<std::string>& args,
                      const std::initializer_list<std::string>& envVars)
{
    std::string command;
    for (const auto& envVar : envVars)
    {
        command += envVar;
        command += ' ';
    }
    command += "cd " + shellQuote(workingDir.string()) + " && " + shellQuote(BEEZ_EXECUTABLE);
    for (const auto& argument : args)
    {
        command += ' ';
        command += shellQuote(argument);
    }
    command += " 2>&1";

    std::string output;
    std::array<char, 256> buffer {};

    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe)
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        return {.exitCode = -1, .output = "failed to start beez process"};
    }

    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }

    const int Status = pclose(pipe);
    if (Status == -1)
    {
        return finalizeProcessStatus(-1, output + "\nfailed to close beez process");
    }

    return finalizeProcessStatus(Status, std::move(output));
}

ProcessResult runShellScript(const std::filesystem::path& scriptPath,
                             const std::initializer_list<std::string>& envVars)
{
    std::string command;
    for (const auto& envVar : envVars)
    {
        command += envVar;
        command += ' ';
    }
    command += shellQuote("bash") + ' ' + shellQuote(scriptPath.string()) + " 2>&1";

    std::string output;
    std::array<char, 256> buffer {};

    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe)
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        return {.exitCode = -1, .output = "failed to start shell script"};
    }

    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }

    const int Status = pclose(pipe);
    if (Status == -1)
    {
        return finalizeProcessStatus(-1, output + "\nfailed to close shell script");
    }

    return finalizeProcessStatus(Status, std::move(output));
}

}  // namespace beez::test
// NOLINTEND(misc-include-cleaner)
