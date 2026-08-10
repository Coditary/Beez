#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace beez::plugin::lua::api_detail
{

[[nodiscard]] inline std::filesystem::path resolvePath(const std::filesystem::path& projectRoot,
                                                     const std::string& userPath)
{
    const std::filesystem::path UserPath(userPath);
    if (UserPath.is_absolute())
    {
        return UserPath;
    }

    return projectRoot / UserPath;
}

[[nodiscard]] inline std::string joinPathSegments(const std::vector<std::string>& segments)
{
    std::filesystem::path result;
    for (const auto& segment : segments)
    {
        result /= segment;
    }

    return result.generic_string();
}

}  // namespace beez::plugin::lua::api_detail
