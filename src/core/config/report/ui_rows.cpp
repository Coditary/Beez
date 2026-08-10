#include "detail/report_helpers.hpp"

#include "beez/core/config/report/settings_report.hpp"
#include "beez/core/config/ui/resolve.hpp"
#include "beez/core/config/ui/types.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include <optional>
#include <string>
#include <vector>

namespace beez::core::settings_report
{
void appendUiRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto& cli = input.cliOptions;

    const logging::OutputMode ResolvedOutputMode =
        active.ui.outputMode.value_or(logging::OutputMode::Clean);
    const UiSettings ResolvedUi = active.resolveUiSettings();
    const auto [OutputModeCliOverride, OutputModeCliLabel] = outputModeCliOrigin(cli);

    rows.push_back(ConfigRow {
        .key = "ui.output_mode",
        .value = formatQuoted(outputModeLabel(ResolvedOutputMode)),
        .origin = originForOptional(global.ui.outputMode,
                                    project.ui.outputMode,
                                    OutputModeCliOverride,
                                    OutputModeCliLabel,
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.colors",
        .value = formatBool(ResolvedUi.colors),
        .origin = originForOptional(global.ui.options.colors,
                                    project.ui.options.colors,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.truecolor",
        .value = formatBool(ResolvedUi.truecolor),
        .origin = originForOptional(global.ui.options.truecolor,
                                    project.ui.options.truecolor,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.icons",
        .value = formatBool(ResolvedUi.icons),
        .origin = originForOptional(global.ui.options.icons,
                                    project.ui.options.icons,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.animation.progress",
        .value = formatQuoted(toString(ResolvedUi.animation.progress.style)),
        .origin = defaultOriginLabel(),
    });
    rows.push_back(ConfigRow {
        .key = "ui.animation.indicator",
        .value = formatQuoted(toString(ResolvedUi.animation.progress.indicator)),
        .origin = defaultOriginLabel(),
    });
    rows.push_back(ConfigRow {
        .key = "ui.animation.indicator_spin_interval",
        .value = std::to_string(ResolvedUi.animation.progress.indicatorSpinIntervalMs),
        .origin = defaultOriginLabel(),
    });
    rows.push_back(ConfigRow {
        .key = "ui.log_level",
        .value = formatQuoted(toString(ResolvedUi.logLevel)),
        .origin = originForOptional(global.ui.options.logLevel,
                                    project.ui.options.logLevel,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.hide_cache_hits",
        .value = formatBool(ResolvedUi.hideCacheHits),
        .origin = originForOptional(global.ui.options.hideCacheHits,
                                    project.ui.options.hideCacheHits,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.prefix",
        .value = formatBool(ResolvedUi.workerPrefixEnabled),
        .origin = originForOptional(global.ui.options.workerPrefix,
                                    project.ui.options.workerPrefix,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.show_time_saved",
        .value = formatBool(ResolvedUi.showTimeSaved),
        .origin = originForOptional(global.ui.options.showTimeSaved,
                                    project.ui.options.showTimeSaved,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.summary",
        .value = formatQuoted(toString(ResolvedUi.summaryStyle)),
        .origin = originForOptional(global.ui.options.summary,
                                    project.ui.options.summary,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    const logging::LoggingSettings ResolvedLogging = active.resolveLoggingSettings(input.context);
    rows.push_back(ConfigRow {
        .key = "ui.logging.run_log",
        .value = formatBool(ResolvedLogging.runLog),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->runLog
                                                  : std::optional<bool> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->runLog
                                                   : std::optional<bool> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.run_log_file",
        .value = formatQuoted(ResolvedLogging.runLogFile.string()),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->runLogFile
                                                  : std::optional<std::filesystem::path> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->runLogFile
                                                   : std::optional<std::filesystem::path> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.log_steps",
        .value = formatBool(ResolvedLogging.logSteps),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->logSteps
                                                  : std::optional<bool> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->logSteps
                                                   : std::optional<bool> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.workers",
        .value = formatQuoted(logging::toString(ResolvedLogging.workers)),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->workers
                                                  : std::optional<std::string> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->workers
                                                   : std::optional<std::string> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.workers_dir",
        .value = formatQuoted(ResolvedLogging.workersDir.string()),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->workersDir
                                                  : std::optional<std::filesystem::path> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->workersDir
                                                   : std::optional<std::filesystem::path> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
}
}  // namespace beez::core::settings_report
