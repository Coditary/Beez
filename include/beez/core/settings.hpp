#pragma once

#include "beez/cli/parsed_options.hpp"
#include "beez/core/context.h"
#include "beez/core/run_options.hpp"
#include "beez/logging/output_mode.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace beez::logging
{
class ILogger;
}  // namespace beez::logging

namespace beez::core
{

// NOLINTBEGIN(misc-non-private-member-variables-in-classes) -- configuration aggregate
struct BeezSettings
{
    struct Performance
    {
        std::optional<std::size_t> maxThreads;
    } performance;

    struct Cache
    {
        std::optional<std::filesystem::path> directory;
    } cache;

    struct Ui
    {
        std::optional<logging::OutputMode> outputMode;
    } ui;

    struct Paths
    {
        std::optional<std::filesystem::path> envFile;
        std::optional<std::string> buildScript;
    } paths;

    struct Engine
    {
        std::optional<bool> dryRun;
        std::optional<bool> enableCache;
    } engine;

    std::unordered_map<std::string, std::string> environment;

    void merge(const BeezSettings& overlay);

    void applyEnvironment() const;

    void applyCliOverrides(const cli::ParsedOptions& options);

    void applyToContext(Context& context) const;

    [[nodiscard]] RunOptions toRunOptions(logging::ILogger* logger, const Context& context) const;

    [[nodiscard]] std::filesystem::path resolveCacheDirectory(const Context& context) const;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
