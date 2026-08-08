#pragma once

#include "beez/core/text_table.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ratio>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace beez::perf
{

struct TimingSample
{
    std::string scenario;
    std::string suite;
    std::size_t virtualFiles = 0;
    std::size_t materializedFiles = 0;
    std::size_t steps = 0;
    double orderMs = 0.0;
    double registryMs = 0.0;
    double orchestratorMs = 0.0;
};

class TimingReport
{
  public:
    static TimingReport& instance()
    {
        static TimingReport report;
        return report;
    }

    void record(TimingSample sample)
    {
        samples_.push_back(std::move(sample));
    }

    void write(const std::filesystem::path& reportPath) const
    {
        if (samples_.empty())
        {
            return;
        }

        std::error_code errorCode;
        std::filesystem::create_directories(reportPath.parent_path(), errorCode);

        std::ostringstream stream;
        stream << "Beez performance throughput report\n";
        stream << "==================================\n\n";

        core::TextTable table({"scenario",
                               "files",
                               "materialized",
                               "steps",
                               "order_ms",
                               "registry_ms",
                               "orchestrator_ms",
                               "suite"});
        for (const auto& sample : samples_)
        {
            std::ostringstream timing;
            timing << std::fixed << std::setprecision(3);
            timing << sample.orderMs;
            const std::string OrderMs = timing.str();

            timing.str({});
            timing.clear();
            timing << sample.registryMs;
            const std::string RegistryMs = timing.str();

            timing.str({});
            timing.clear();
            timing << sample.orchestratorMs;
            const std::string OrchestratorMs = timing.str();

            table.addRow({sample.scenario,
                          std::to_string(sample.virtualFiles),
                          std::to_string(sample.materializedFiles),
                          std::to_string(sample.steps),
                          OrderMs,
                          RegistryMs,
                          OrchestratorMs,
                          sample.suite});
        }

        stream << table.format();
        stream << "\n\nNotes:\n";
        stream << "- Virtual files are catalogued paths; only small scenarios materialize files on "
                  "disk.\n";
        stream << "- DAG resolution uses glob-pattern overlap (no directory traversal).\n";
        stream << "- orchestrator_ms uses dry-run (graph build + ordering, no shell execution).\n";

        const auto ReportText = stream.str();
        std::cout << '\n' << ReportText;

        std::ofstream file(reportPath);
        if (file.is_open())
        {
            file << ReportText;
        }
    }

  private:
    std::vector<TimingSample> samples_;
};

inline double elapsedMs(const std::chrono::steady_clock::time_point& start,
                        const std::chrono::steady_clock::time_point& end)
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace beez::perf
