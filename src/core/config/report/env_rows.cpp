#include "detail/report_helpers.hpp"

#include "beez/core/config/env/env_settings.hpp"
#include "beez/core/config/report/settings_report.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/runtime/context.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace beez::core::settings_report
{
[[nodiscard]] std::string formatStringList(const std::vector<std::string>& values)
{
    if (values.empty())
    {
        return "[]";
    }

    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0U)
        {
            stream << ", ";
        }
        stream << formatQuoted(values.at(index));
    }
    stream << ']';
    return stream.str();
}

[[nodiscard]] std::string formatPathList(const std::vector<std::filesystem::path>& paths)
{
    if (paths.empty())
    {
        return "[]";
    }

    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < paths.size(); ++index)
    {
        if (index > 0U)
        {
            stream << ", ";
        }
        stream << '"' << formatDisplayPath(paths.at(index)) << '"';
    }
    stream << ']';
    return stream.str();
}

[[nodiscard]] std::string envVarOriginForKey(const std::string& key,
                                             const BeezSettings& global,
                                             const BeezSettings& project,
                                             const std::filesystem::path& globalConfigPath,
                                             const Context& context)
{
    const bool InProject = project.env.vars.contains(key);
    const bool InGlobal = global.env.vars.contains(key);

    if (InProject)
    {
        return projectOriginLabel(context);
    }

    if (InGlobal)
    {
        return globalOriginLabel(globalConfigPath);
    }

    return defaultOriginLabel();
}

[[nodiscard]] std::string originForProjectOrGlobal(bool inProject,
                                                   bool inGlobal,
                                                   const std::filesystem::path& globalConfigPath,
                                                   const Context& context)
{
    if (inProject)
    {
        return projectOriginLabel(context);
    }

    if (inGlobal)
    {
        return globalOriginLabel(globalConfigPath);
    }

    return defaultOriginLabel();
}

void appendEnvRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto ResolvedEnv = active.resolveEnvSettings();

    rows.push_back(ConfigRow {
        .key = "env.load_dotenv",
        .value = formatBool(ResolvedEnv.loadDotenv),
        .origin = originForOptional(global.env.loadDotenv,
                                    project.env.loadDotenv,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.dotenv_overrides_system",
        .value = formatBool(ResolvedEnv.dotenvOverridesSystem),
        .origin = originForOptional(global.env.dotenvOverridesSystem,
                                    project.env.dotenvOverridesSystem,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.files",
        .value = formatPathList(ResolvedEnv.files),
        .origin = originForProjectOrGlobal(!project.env.files.empty(),
                                           !global.env.files.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.hash_vars",
        .value = formatStringList(ResolvedEnv.hashVars),
        .origin = originForProjectOrGlobal(!project.env.hashVars.empty(),
                                           !global.env.hashVars.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.ignore_vars_for_hashing",
        .value = formatStringList(ResolvedEnv.ignoreVarsForHashing),
        .origin = originForProjectOrGlobal(!project.env.ignoreVarsForHashing.empty(),
                                           !global.env.ignoreVarsForHashing.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.mask_secrets",
        .value = formatStringList(ResolvedEnv.maskSecrets),
        .origin = originForProjectOrGlobal(!project.env.maskSecrets.empty(),
                                           !global.env.maskSecrets.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });

    std::vector<std::string> keys;
    keys.reserve(ResolvedEnv.vars.size());
    for (const auto& [key, value] : ResolvedEnv.vars)
    {
        keys.push_back(key);
    }
    std::ranges::sort(keys);

    for (const auto& key : keys)
    {
        const auto& value = ResolvedEnv.vars.at(key);
        rows.push_back(ConfigRow {
            .key = "env.vars." + key,
            .value = formatQuoted(value),
            .origin = envVarOriginForKey(key,
                                         input.globalSettings,
                                         input.projectSettings,
                                         input.globalConfigPath,
                                         input.context),
        });
    }
}
}  // namespace beez::core::settings_report
