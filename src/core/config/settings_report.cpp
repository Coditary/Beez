#include "beez/core/config/settings_report.hpp"

#include "report/report_helpers.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace beez::core
{

std::string formatActiveConfiguration(const SettingsReportInput& input)
{
    std::ostringstream stream;
    stream << "=== Beez Active Configuration ===\n\n";

    std::vector<settings_report::ConfigRow> performanceRows;
    settings_report::appendPerformanceRows(input, performanceRows);
    settings_report::appendSection(stream, "Performance", performanceRows);

    std::vector<settings_report::ConfigRow> cacheRows;
    settings_report::appendCacheRows(input, cacheRows);
    settings_report::appendSection(stream, "Cache", cacheRows);

    std::vector<settings_report::ConfigRow> uiRows;
    settings_report::appendUiRows(input, uiRows);
    settings_report::appendSection(stream, "UI", uiRows);

    std::vector<settings_report::ConfigRow> envRows;
    settings_report::appendEnvRows(input, envRows);
    settings_report::appendSection(stream, "Env", envRows);

    std::string output = stream.str();
    if (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }

    return output;
}

}  // namespace beez::core
