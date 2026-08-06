#pragma once

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace beez::perf
{

inline std::filesystem::path performanceRoot()
{
#ifdef BEEZ_PERF_ROOT_DIR
    return {BEEZ_PERF_ROOT_DIR};
#else
    return std::filesystem::current_path() / "test" / "performance";
#endif
}

inline std::filesystem::path performanceReportPath()
{
#ifdef BEEZ_PERF_REPORT_DIR
    return std::filesystem::path(BEEZ_PERF_REPORT_DIR) / "throughput-report.txt";
#else
    return performanceRoot() / "reports" / "throughput-report.txt";
#endif
}

class PerformanceWorkspace
{
  public:
    explicit PerformanceWorkspace(std::string scenarioName)
        : path_(performanceRoot() / "artifact-vault" / std::move(scenarioName))
    {
        std::error_code errorCode;
        std::filesystem::remove_all(path_, errorCode);
        std::filesystem::create_directories(path_, errorCode);
    }

    PerformanceWorkspace(const PerformanceWorkspace&) = delete;
    PerformanceWorkspace& operator=(const PerformanceWorkspace&) = delete;

    ~PerformanceWorkspace()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(path_, errorCode);
    }

    [[nodiscard]] const std::filesystem::path& path() const
    {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

}  // namespace beez::perf
