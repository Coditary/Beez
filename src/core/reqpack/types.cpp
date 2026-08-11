#include "beez/core/reqpack/types.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace beez::core
{

bool ReqPackInstallResponse::succeeded() const
{
    if (ok.has_value() && !*ok)
    {
        return false;
    }

    return std::ranges::none_of(
        plugins,
        [](const auto& entry)
        {
            return std::ranges::any_of(
                entry.second, [](const ReqPackPackageResult& package) { return package.failed(); });
        });
}

ReqPackManifest ReqPackInstallResponse::successfulPackages() const
{
    ReqPackManifest manifest;
    for (const auto& [plugin, packages] : plugins)
    {
        std::vector<ReqPackPackage> successful;
        for (const auto& package : packages)
        {
            if (package.failed())
            {
                continue;
            }

            ReqPackPackage entry {.name = package.name};
            if (package.version.has_value() && !package.version->empty())
            {
                entry.version = package.version;
            }
            successful.push_back(std::move(entry));
        }

        if (!successful.empty())
        {
            manifest.plugins.emplace(plugin, std::move(successful));
        }
    }
    return manifest;
}

}  // namespace beez::core
