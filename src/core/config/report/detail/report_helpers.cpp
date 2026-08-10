#include "detail/report_helpers.hpp"

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/text_table.hpp"
#include "beez/logging/console/output_mode.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace beez::core::settings_report
{

namespace
{

[[nodiscard]] std::string homeDirectory()
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        return home;
    }

    return {};
}

}  // namespace

std::string formatDisplayPath(const std::filesystem::path& path)
{
    const std::string Home = homeDirectory();
    std::string native = path.string();
    if (!Home.empty() && native.starts_with(Home))
    {
        return std::string("~") + native.substr(Home.size());
    }

    return native;
}

std::string formatOptionalPath(const std::optional<std::filesystem::path>& path)
{
    if (!path.has_value())
    {
        return "<unset>";
    }

    return '"' + formatDisplayPath(*path) + '"';
}

std::string formatBool(bool value)
{
    return value ? "true" : "false";
}

std::string formatQuoted(const std::string& value)
{
    return '"' + value + '"';
}

std::string outputModeLabel(logging::OutputMode mode)
{
    return logging::outputModeToString(mode);
}

std::pair<bool, std::string> outputModeCliOrigin(const cli::ParsedOptions& cli)
{
    if (cli.silent)
    {
        return {true, "CLI --silent"};
    }

    if (cli.errorsOnly)
    {
        return {true, "CLI --error"};
    }

    if (cli.verbose)
    {
        return {true, "CLI --verbose"};
    }

    return {false, {}};
}

std::string defaultOriginLabel()
{
    return "Default";
}

std::string globalOriginLabel(const std::filesystem::path& globalConfigPath)
{
    if (globalConfigPath.empty())
    {
        return defaultOriginLabel();
    }

    return formatDisplayPath(globalConfigPath);
}

std::string projectOriginLabel(const Context& context)
{
    return formatDisplayPath(context.buildScriptPath().filename());
}

void appendSection(std::ostringstream& stream,
                   const std::string& title,
                   const std::vector<ConfigRow>& rows)
{
    if (rows.empty())
    {
        return;
    }

    stream << '[' << title << "]\n";

    TextTable table({"Setting", "Value", "Origin"});
    for (const auto& row : rows)
    {
        table.addRow({row.key, row.value, row.origin});
    }

    stream << table.format() << "\n\n";
}

}  // namespace beez::core::settings_report
