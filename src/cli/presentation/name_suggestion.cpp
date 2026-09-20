#include "beez/cli/presentation/name_suggestion.hpp"

#include "beez/core/registry/registry.hpp"

#include "completion/fuzzy.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace beez::cli
{

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
    return completion::suggest_similar_name(query, candidates);
}

std::string formatDidYouMean(const std::string& suggestion)
{
    return completion::format_did_you_mean(suggestion);
}

}  // namespace beez::cli
