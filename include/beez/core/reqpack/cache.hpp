#pragma once

#include "beez/core/reqpack/types.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace beez::core
{

[[nodiscard]] std::string pluginFingerprint(const std::vector<ReqPackPackage>& packages);

[[nodiscard]] std::filesystem::path reqpackCacheDir(const std::filesystem::path& projectRoot);

[[nodiscard]] std::map<std::string, std::vector<ReqPackPackage>>
filterUncachedPlugins(const ReqPackManifest& manifest, const std::filesystem::path& projectRoot);

void updatePluginCache(const std::string& plugin,
                       const std::vector<ReqPackPackage>& packages,
                       const std::filesystem::path& projectRoot);

}  // namespace beez::core
