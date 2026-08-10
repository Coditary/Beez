#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config/report/settings_report.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/logging/console/output_mode.hpp"

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace beez::core::settings_report
{

struct ConfigRow
{
    std::string key;
    std::string value;
    std::string origin;
};

void appendSection(std::ostringstream& stream,
                   const std::string& title,
                   const std::vector<ConfigRow>& rows);

[[nodiscard]] std::string formatDisplayPath(const std::filesystem::path& path);

[[nodiscard]] std::string formatOptionalPath(const std::optional<std::filesystem::path>& path);

[[nodiscard]] std::string formatBool(bool value);

[[nodiscard]] std::string formatQuoted(const std::string& value);

[[nodiscard]] std::string outputModeLabel(logging::OutputMode mode);

[[nodiscard]] std::pair<bool, std::string> outputModeCliOrigin(const cli::ParsedOptions& cli);

[[nodiscard]] std::string defaultOriginLabel();

[[nodiscard]] std::string globalOriginLabel(const std::filesystem::path& globalConfigPath);

[[nodiscard]] std::string projectOriginLabel(const Context& context);

template <typename T>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- layers are semantically distinct
[[nodiscard]] std::string originForOptional(const std::optional<T>& globalValue,
                                            const std::optional<T>& projectValue,
                                            bool cliOverride,
                                            const std::string& cliLabel,
                                            const std::filesystem::path& globalConfigPath,
                                            const Context& context)
{
    if (cliOverride)
    {
        return cliLabel;
    }

    if (projectValue.has_value())
    {
        return projectOriginLabel(context);
    }

    if (globalValue.has_value())
    {
        return globalOriginLabel(globalConfigPath);
    }

    return defaultOriginLabel();
}

[[nodiscard]] std::string formatStringList(const std::vector<std::string>& values);

[[nodiscard]] std::string formatPathList(const std::vector<std::filesystem::path>& paths);

[[nodiscard]] std::string envVarOriginForKey(const std::string& key,
                                             const BeezSettings& global,
                                             const BeezSettings& project,
                                             const std::filesystem::path& globalConfigPath,
                                             const Context& context);

[[nodiscard]] std::string originForProjectOrGlobal(bool inProject,
                                                   bool inGlobal,
                                                   const std::filesystem::path& globalConfigPath,
                                                   const Context& context);

void appendPerformanceRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows);

void appendCacheRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows);

void appendUiRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows);

void appendEnvRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows);

}  // namespace beez::core::settings_report
