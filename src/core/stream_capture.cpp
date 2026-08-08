#include "beez/core/stream_capture.hpp"

#include <array>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner)
#include <fcntl.h>
#include <unistd.h>
// NOLINTEND(misc-include-cleaner)

namespace beez::core
{

namespace
{

constexpr std::size_t CaptureBufferSize = 256;
constexpr std::size_t PipeReadEnd = 0;
constexpr std::size_t PipeWriteEnd = 1;

void restoreDescriptor(int savedDescriptor, int targetDescriptor)
{
    if (savedDescriptor < 0)
    {
        return;
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    dup2(savedDescriptor, targetDescriptor);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    close(savedDescriptor);
}

}  // namespace

CapturedExecution captureProcessOutput(const std::function<int()>& action)
{
    std::array<int, 2> pipeFds = {-1, -1};
    if (pipe(pipeFds.data()) != 0)
    {
        return {.exitCode = action()};
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const int SavedStdout = dup(STDOUT_FILENO);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const int SavedStderr = dup(STDERR_FILENO);

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    dup2(pipeFds.at(PipeWriteEnd), STDOUT_FILENO);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    dup2(pipeFds.at(PipeWriteEnd), STDERR_FILENO);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    close(pipeFds.at(PipeWriteEnd));

    const int ExitCode = action();

    // NOLINTNEXTLINE(cert-err33-c,concurrency-mt-unsafe)
    static_cast<void>(fflush(stdout));
    // NOLINTNEXTLINE(cert-err33-c,concurrency-mt-unsafe)
    static_cast<void>(fflush(stderr));

    restoreDescriptor(SavedStdout, STDOUT_FILENO);
    restoreDescriptor(SavedStderr, STDERR_FILENO);

    std::string output;
    std::array<char, CaptureBufferSize> buffer {};
    long bytesRead = 0;
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    while ((bytesRead = read(pipeFds.at(PipeReadEnd), buffer.data(), buffer.size())) > 0)
    {
        output.append(buffer.data(), static_cast<std::size_t>(bytesRead));
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    close(pipeFds.at(PipeReadEnd));
    return {.exitCode = ExitCode, .output = std::move(output)};
}

int discardProcessOutput(const std::function<int()>& action)
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const int SavedStdout = dup(STDOUT_FILENO);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const int SavedStderr = dup(STDERR_FILENO);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,concurrency-mt-unsafe)
    const int DevNull = ::open("/dev/null", O_WRONLY);
    if (DevNull < 0)
    {
        restoreDescriptor(SavedStdout, STDOUT_FILENO);
        restoreDescriptor(SavedStderr, STDERR_FILENO);
        return action();
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    dup2(DevNull, STDOUT_FILENO);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    dup2(DevNull, STDERR_FILENO);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    close(DevNull);

    const int ExitCode = action();

    // NOLINTNEXTLINE(cert-err33-c,concurrency-mt-unsafe)
    static_cast<void>(fflush(stdout));
    // NOLINTNEXTLINE(cert-err33-c,concurrency-mt-unsafe)
    static_cast<void>(fflush(stderr));

    restoreDescriptor(SavedStdout, STDOUT_FILENO);
    restoreDescriptor(SavedStderr, STDERR_FILENO);
    return ExitCode;
}

}  // namespace beez::core
