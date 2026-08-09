#include "beez/plugin/shell/shell_executor.hpp"

#include "beez/core/runtime/context.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#include <cstdio>
// NOLINTEND(misc-include-cleaner)

// NOLINTBEGIN(misc-include-cleaner)
#include <sys/wait.h>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::shell
{

namespace
{

constexpr std::size_t CaptureBufferSize = 256;

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
    quoted += "'";
    return quoted;
}

std::string wrapCommand(const std::string& command, const core::Context& context)
{
    return "cd " + shellQuote(context.projectRoot().string()) + " && " + command;
}

int executeWithoutCapture(const std::string& command)
{
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe)
    const int Status = std::system(command.c_str());
    if (Status == -1)
    {
        return -1;
    }

    if (WIFEXITED(Status))  // NOLINT(misc-include-cleaner)
    {
        return WEXITSTATUS(Status);  // NOLINT(misc-include-cleaner)
    }

    return -1;
}

int executeWithCapture(const std::string& command, std::string& capturedOutput)
{
    const std::string ShellCommand = "(" + command + ") 2>&1";
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe,misc-include-cleaner)
    FILE* pipe = popen(ShellCommand.c_str(), "r");
    if (pipe == nullptr)
    {
        return -1;
    }

    std::array<char, CaptureBufferSize> buffer {};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        capturedOutput += buffer.data();
    }

    // NOLINTNEXTLINE(misc-include-cleaner)
    const int Status = pclose(pipe);
    if (Status == -1)
    {
        return -1;
    }

    if (WIFEXITED(Status))
    {
        return WEXITSTATUS(Status);
    }

    return -1;
}

}  // namespace

int ShellExecutor::execute(const std::string& command,
                           const core::Context& context,
                           std::string* capturedOutput)
{
    const std::string WrappedCommand = wrapCommand(command, context);
    if (capturedOutput != nullptr)
    {
        capturedOutput->clear();
        return executeWithCapture(WrappedCommand, *capturedOutput);
    }

    return executeWithoutCapture(WrappedCommand);
}

std::string ShellPlugin::name() const
{
    return "shell";
}

void ShellPlugin::registerCapabilities(PluginHost& host)
{
    host.setExecutor(std::make_unique<ShellExecutor>());
}

}  // namespace beez::plugin::shell
