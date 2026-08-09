#include "beez/cli/commands/config.hpp"

#include "beez/cli/completion/install_completion.hpp"
#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config/config_schema.hpp"
#include "beez/core/config/settings_report.hpp"

#include <iostream>
#include <optional>

namespace beez::cli
{

std::optional<int> runEarlyConfigCommands(const ParsedOptions& options)
{
    if (options.completeConfigOptions)
    {
        for (const auto& path :
             core::listConfigOptionCompletions(options.completeConfigOptionsPrefix))
        {
            std::cout << path << '\n';
        }
        return 0;
    }

    if (options.dumpCompletion)
    {
        const auto Script = dumpCompletionScript(options.dumpCompletionShell);
        if (!Script.has_value())
        {
            std::cerr << "Error: unknown shell for --dump-completion: "
                      << options.dumpCompletionShell << '\n';
            return 1;
        }

        std::cout << *Script;
        return 0;
    }

    if (options.configOptions)
    {
        const auto Output = core::formatConfigOptions(options.configOptionsPath);
        if (!Output.has_value())
        {
            std::cerr << "Error: unknown config option path: " << options.configOptionsPath << '\n';
            return 1;
        }

        std::cout << *Output << '\n';
        return 0;
    }

    return std::nullopt;
}

std::optional<int> runShowConfigCommand(const ParsedOptions& options,
                                        const core::SettingsReportInput& input)
{
    if (!options.showConfig)
    {
        return std::nullopt;
    }

    std::cout << core::formatActiveConfiguration(input) << '\n';
    return 0;
}

}  // namespace beez::cli
