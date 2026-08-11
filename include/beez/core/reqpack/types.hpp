#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

// NOLINTBEGIN(misc-non-private-member-variables-in-classes) -- reqpack data aggregate
struct ReqPackPackage
{
    std::string name;
    std::optional<std::string> version;

    [[nodiscard]] bool operator==(const ReqPackPackage& other) const = default;
};

struct ReqPackManifest
{
    std::map<std::string, std::vector<ReqPackPackage>> plugins;

    [[nodiscard]] bool empty() const
    {
        return plugins.empty();
    }

    [[nodiscard]] bool operator==(const ReqPackManifest& other) const = default;
};

struct ReqPackPackageResult
{
    std::string name;
    std::optional<std::string> version;
    std::optional<std::string> status;
    std::optional<std::string> errorMessage;

    [[nodiscard]] bool failed() const
    {
        return status.has_value() && *status == "failed";
    }
};

struct ReqPackInstallResponse
{
    std::optional<bool> ok;
    bool dryRun = false;
    std::map<std::string, std::vector<ReqPackPackageResult>> plugins;

    [[nodiscard]] bool succeeded() const;

    [[nodiscard]] ReqPackManifest successfulPackages() const;
};

// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
