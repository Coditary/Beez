#include "beez/logging/worker_output_format.hpp"

// NOLINTBEGIN(misc-include-cleaner) -- TIOCGWINSZ and winsize come from platform ioctl headers
#include <sys/ioctl.h>
#include <unistd.h>
// NOLINTEND(misc-include-cleaner)

#include <cstddef>
#include <string_view>
#include <vector>

namespace beez::logging
{

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-type-vararg)
std::size_t stdoutTerminalWidth(const std::size_t Fallback)
{
    winsize size {};
    if (isatty(STDOUT_FILENO) != 0 && ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 &&
        size.ws_col > 0)
    {
        return size.ws_col;
    }

    return Fallback;
}
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-type-vararg)

std::vector<std::string_view> splitWorkerOutputLine(const std::string_view Line,
                                                    const std::size_t TerminalWidth)
{
    std::vector<std::string_view> segments;
    if (Line.empty())
    {
        return segments;
    }

    if (TerminalWidth <= WorkerOutputPrefix.size())
    {
        segments.push_back(Line);
        return segments;
    }

    const std::size_t ChunkSize = TerminalWidth - WorkerOutputPrefix.size();
    for (std::size_t offset = 0; offset < Line.size(); offset += ChunkSize)
    {
        segments.push_back(Line.substr(offset, ChunkSize));
    }

    return segments;
}

}  // namespace beez::logging
