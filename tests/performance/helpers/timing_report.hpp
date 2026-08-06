#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
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
        stream << std::fixed << std::setprecision(3);
        stream << "Beez performance throughput report\n";
        stream << "==================================\n\n";
        stream << std::left << std::setw(12) << "scenario" << std::setw(10) << "files"
               << std::setw(12) << "materialized" << std::setw(8) << "steps" << std::setw(14)
               << "order_ms" << std::setw(14) << "registry_ms" << std::setw(16) << "orchestrator_ms"
               << "suite\n";
        stream << std::string(96, '-') << '\n';

        for (const auto& sample : samples_)
        {
            stream << std::setw(12) << sample.scenario << std::setw(10) << sample.virtualFiles
                   << std::setw(12) << sample.materializedFiles << std::setw(8) << sample.steps
                   << std::setw(14) << sample.orderMs << std::setw(14) << sample.registryMs
                   << std::setw(16) << sample.orchestratorMs << sample.suite << '\n';
        }

        stream << "\nNotes:\n";
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
