// NOLINTBEGIN(misc-include-cleaner,readability-identifier-length,readability-identifier-naming)
#include "beez/logging/backends/spdlog_setup.hpp"

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>

namespace beez::logging
{

namespace
{

[[nodiscard]] spdlog::level::level_enum toSpdlogLevel(const core::UiLogLevel level)
{
    switch (level)
    {
    case core::UiLogLevel::Warn:
        return spdlog::level::warn;
    case core::UiLogLevel::Error:
        return spdlog::level::err;
    case core::UiLogLevel::Info:
        break;
    }

    return spdlog::level::info;
}

}  // namespace

std::shared_ptr<spdlog::logger> makeConsoleLogger(const core::UiSettings& uiSettings)
{
    // Drop first: spdlog throws if a logger with this name is already registered.
    spdlog::drop("beez");
    auto logger = spdlog::stdout_color_mt("beez");
    logger->set_pattern("%v");
    logger->set_level(toSpdlogLevel(uiSettings.logLevel));
    return logger;
}

std::shared_ptr<spdlog::logger> makeFileLogger(const std::filesystem::path& logFile,
                                               const core::UiSettings& uiSettings)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logFile.parent_path(), errorCode);

    spdlog::drop("beez_file");
    auto logger = spdlog::basic_logger_mt("beez_file", logFile.string(), true);
    logger->set_pattern("%v");
    logger->set_level(toSpdlogLevel(uiSettings.logLevel));
    return logger;
}

}  // namespace beez::logging
// NOLINTEND(misc-include-cleaner,readability-identifier-length,readability-identifier-naming)
