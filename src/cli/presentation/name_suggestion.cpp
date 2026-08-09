#include "beez/cli/presentation/name_suggestion.hpp"

#include "beez/core/registry.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace beez::cli
{

namespace
{

[[nodiscard]] std::size_t levenshteinDistance(const std::string& left, const std::string& right)
{
    const std::size_t LeftSize = left.size();
    const std::size_t RightSize = right.size();

    if (LeftSize == 0)
    {
        return RightSize;
    }

    if (RightSize == 0)
    {
        return LeftSize;
    }

    std::vector<std::size_t> previous(RightSize + 1);
    std::vector<std::size_t> current(RightSize + 1);

    for (std::size_t column = 0; column <= RightSize; ++column)
    {
        previous.at(column) = column;
    }

    for (std::size_t row = 1; row <= LeftSize; ++row)
    {
        current.at(0) = row;
        for (std::size_t column = 1; column <= RightSize; ++column)
        {
            const std::size_t SubstitutionCost = left.at(row - 1) == right.at(column - 1) ? 0 : 1;
            current.at(column) = std::min({previous.at(column) + 1,
                                           current.at(column - 1) + 1,
                                           previous.at(column - 1) + SubstitutionCost});
        }
        previous.swap(current);
    }

    return previous.at(RightSize);
}

[[nodiscard]] std::size_t maxEditDistance(const std::string& query)
{
    return std::min<std::size_t>(3, std::max<std::size_t>(2, query.size() / 4));
}

}  // namespace

std::vector<std::string> collectRunnableNames(const core::Registry& registry)
{
    std::vector<std::string> names;
    names.reserve(registry.tasks().size() + registry.workflows().size());

    for (const auto& [name, task] : registry.tasks())
    {
        (void)task;
        names.push_back(name);
    }

    for (const auto& [name, workflow] : registry.workflows())
    {
        (void)workflow;
        names.push_back(name);
    }

    std::ranges::sort(names);
    return names;
}

std::optional<std::string> suggestSimilarName(const std::string& query,
                                              const std::vector<std::string>& candidates)
{
    if (query.empty() || candidates.empty())
    {
        return std::nullopt;
    }

    const std::size_t AllowedDistance = maxEditDistance(query);
    std::optional<std::string> bestMatch;
    std::size_t bestDistance = std::numeric_limits<std::size_t>::max();

    for (const auto& candidate : candidates)
    {
        if (candidate == query)
        {
            continue;
        }

        const std::size_t Distance = levenshteinDistance(query, candidate);
        if (Distance > AllowedDistance || Distance >= bestDistance)
        {
            continue;
        }

        bestDistance = Distance;
        bestMatch = candidate;
    }

    return bestMatch;
}

std::string formatDidYouMean(const std::string& suggestion)
{
    return "Did you mean '" + suggestion + "'?";
}

}  // namespace beez::cli
