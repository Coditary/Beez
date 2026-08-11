#pragma once

#include <filesystem>
#include <numeric>
#include <string>
#include <vector>

namespace beez::plugin::lua::api_detail
{

[[nodiscard]] inline std::filesystem::path resolvePath(const std::filesystem::path& projectRoot,
                                                       const std::string& userPath)
{
    std::filesystem::path resolved(userPath);
    if (resolved.is_absolute())
    {
        return resolved;
    }

    return projectRoot / resolved;
}

[[nodiscard]] inline std::string joinPathSegments(const std::vector<std::string>& segments)
{
    return std::accumulate(segments.begin(),
                           segments.end(),
                           std::filesystem::path {},
                           [](const std::filesystem::path& result, const std::string& segment)
                           { return result / segment; })
        .generic_string();
}

}  // namespace beez::plugin::lua::api_detail
