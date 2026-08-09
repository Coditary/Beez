#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config/settings.hpp"
#include "beez/core/runtime/context.hpp"

#include <filesystem>
#include <string>

namespace beez::core
{

struct SettingsReportInput
{
    BeezSettings globalSettings;
    std::filesystem::path globalConfigPath;
    BeezSettings projectSettings;
    BeezSettings activeSettings;
    cli::ParsedOptions cliOptions;
    Context context;
};

[[nodiscard]] std::string formatActiveConfiguration(const SettingsReportInput& input);

}  // namespace beez::core
