#include "beez/core/reqpack/cache.hpp"
#include "beez/core/reqpack/types.hpp"

#include "beez/core/cache/fingerprint/content_hash.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] std::string packageFingerprintLine(const ReqPackPackage& package)
{
    std::ostringstream stream;
    stream << package.name;
    if (package.version.has_value())
    {
        stream << '@' << *package.version;
    }
    return stream.str();
}

[[nodiscard]] std::filesystem::path pluginCacheFile(const std::filesystem::path& cacheDir,
                                                    const std::string& plugin)
{
    return cacheDir / (plugin + ".fp");
}

[[nodiscard]] std::string readCachedFingerprint(const std::filesystem::path& cacheFile)
{
    std::ifstream stream(cacheFile);
    if (!stream)
    {
        return {};
    }

    std::string fingerprint;
    std::getline(stream, fingerprint);
    return fingerprint;
}

void writeCachedFingerprint(const std::filesystem::path& cacheFile, std::string_view fingerprint)
{
    std::filesystem::create_directories(cacheFile.parent_path());
    std::ofstream stream(cacheFile, std::ios::trunc);
    stream << fingerprint;
}

}  // namespace

std::string pluginFingerprint(const std::vector<ReqPackPackage>& packages)
{
    const auto Hasher = makeSha256Hasher();
    std::ostringstream stream;
    for (const auto& package : packages)
    {
        stream << packageFingerprintLine(package) << '\0';
    }
    return Hasher->hashBytes(stream.str());
}

std::filesystem::path reqpackCacheDir(const std::filesystem::path& projectRoot)
{
    return projectRoot / ".cache" / "reqpack";
}

std::map<std::string, std::vector<ReqPackPackage>>
filterUncachedPlugins(const ReqPackManifest& manifest, const std::filesystem::path& projectRoot)
{
    const auto CacheDir = reqpackCacheDir(projectRoot);
    std::map<std::string, std::vector<ReqPackPackage>> uncached;

    for (const auto& [plugin, packages] : manifest.plugins)
    {
        if (packages.empty())
        {
            continue;
        }

        const auto Fingerprint = pluginFingerprint(packages);
        const auto Cached = readCachedFingerprint(pluginCacheFile(CacheDir, plugin));
        if (Cached == Fingerprint)
        {
            continue;
        }

        uncached.emplace(plugin, packages);
    }

    return uncached;
}

void updatePluginCache(const std::string& plugin,
                       const std::vector<ReqPackPackage>& packages,
                       const std::filesystem::path& projectRoot)
{
    if (packages.empty())
    {
        return;
    }

    const auto CacheDir = reqpackCacheDir(projectRoot);
    writeCachedFingerprint(pluginCacheFile(CacheDir, plugin), pluginFingerprint(packages));
}

}  // namespace beez::core
