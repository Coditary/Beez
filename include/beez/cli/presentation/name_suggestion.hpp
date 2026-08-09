#pragma once

#include "beez/core/registry/registry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace beez::cli
{

[[nodiscard]] std::vector<std::string> collectRunnableNames(const core::Registry& registry);

[[nodiscard]] std::optional<std::string>
suggestSimilarName(const std::string& query, const std::vector<std::string>& candidates);

[[nodiscard]] std::string formatDidYouMean(const std::string& suggestion);

}  // namespace beez::cli
