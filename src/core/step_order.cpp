#include "beez/core/step_order.hpp"

#include "beez/core/expected.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace beez::core
{

const char* toString(StepOrderErrorKind kind)
{
    switch (kind)
    {
    case StepOrderErrorKind::MutateConflict:
        return "mutate conflict";
    case StepOrderErrorKind::Cycle:
        return "cyclic step dependency";
    }
    return "unknown step order error";
}

namespace
{

using Adjacency = std::unordered_map<std::string, std::set<std::string>>;
using InDegree = std::unordered_map<std::string, std::size_t>;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- left/right semantics are intentional
[[nodiscard]] bool patternsOverlapAny(const std::vector<std::string>& leftPatterns,
                                      const std::vector<std::string>& rightPatterns,
                                      const IGlobMatcher& matcher)
{
    return std::ranges::any_of(leftPatterns,
                               [&rightPatterns, &matcher](const std::string& left)
                               {
                                   return std::ranges::any_of(
                                       rightPatterns,
                                       [&left, &matcher](const std::string& right)
                                       { return matcher.patternsOverlap(left, right); });
                               });
}

void addEdge(const std::string& before,
             const std::string& after,
             Adjacency& adjacency,
             InDegree& inDegree)
{
    if (before == after)
    {
        return;
    }

    if (!adjacency[before].contains(after))
    {
        adjacency[before].insert(after);
        ++inDegree[after];
    }
}

[[nodiscard]] bool hasHint(const std::vector<StepOrderHint>& hints,
                           const std::string& before,
                           const std::string& after)
{
    return std::ranges::any_of(hints,
                               [&before, &after](const StepOrderHint& hint)
                               { return hint.before == before && hint.after == after; });
}

void addArtifactEdges(const Step& step,
                      const Step& other,
                      const IGlobMatcher& matcher,
                      Adjacency& adjacency,
                      InDegree& inDegree)
{
    if (patternsOverlapAny(step.output, other.input, matcher))
    {
        addEdge(step.name, other.name, adjacency, inDegree);
    }

    if (patternsOverlapAny(step.mutate, other.input, matcher))
    {
        addEdge(step.name, other.name, adjacency, inDegree);
    }

    if (patternsOverlapAny(step.output, other.mutate, matcher))
    {
        addEdge(step.name, other.name, adjacency, inDegree);
    }

    if (patternsOverlapAny(step.mutate, other.output, matcher))
    {
        addEdge(other.name, step.name, adjacency, inDegree);
    }
}

[[nodiscard]] std::optional<StepOrderError>
resolveMutateOverlap(const Step& step,
                     const Step& other,
                     const std::vector<StepOrderHint>& hints,
                     Adjacency& adjacency,
                     InDegree& inDegree)
{
    const bool StepBeforeOther = hasHint(hints, step.name, other.name);
    const bool OtherBeforeStep = hasHint(hints, other.name, step.name);

    if (StepBeforeOther && OtherBeforeStep)
    {
        StepOrderError error;
        error.kind = StepOrderErrorKind::MutateConflict;
        error.message = "conflicting order hints between mutate steps '" + step.name + "' and '" +
                        other.name + "'";
        return error;
    }

    if (!StepBeforeOther && !OtherBeforeStep)
    {
        StepOrderError error;
        error.kind = StepOrderErrorKind::MutateConflict;
        error.message = "cannot resolve mutate conflict between '" + step.name + "' and '" +
                        other.name + "'; add order(\"" + step.name + "\", \"" + other.name +
                        "\") or the reverse";
        return error;
    }

    if (StepBeforeOther)
    {
        addEdge(step.name, other.name, adjacency, inDegree);
    }
    else
    {
        addEdge(other.name, step.name, adjacency, inDegree);
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<StepOrderError>
buildDependencyGraph(const std::vector<Step>& steps,
                     const std::vector<StepOrderHint>& hints,
                     const IGlobMatcher& matcher,
                     Adjacency& adjacency,
                     InDegree& inDegree,
                     std::unordered_map<std::string, const Step*>& stepByName)
{
    for (const auto& step : steps)
    {
        stepByName.emplace(step.name, &step);
        inDegree.emplace(step.name, 0);
    }

    for (const auto& step : steps)
    {
        for (const auto& other : steps)
        {
            if (step.name == other.name)
            {
                continue;
            }

            addArtifactEdges(step, other, matcher, adjacency, inDegree);

            if (!step.mutate.empty() && !other.mutate.empty() &&
                patternsOverlapAny(step.mutate, other.mutate, matcher))
            {
                if (const auto Conflict =
                        resolveMutateOverlap(step, other, hints, adjacency, inDegree))
                {
                    return Conflict;
                }
            }
        }
    }

    for (const auto& hint : hints)
    {
        if (!stepByName.contains(hint.before) || !stepByName.contains(hint.after))
        {
            continue;
        }
        addEdge(hint.before, hint.after, adjacency, inDegree);
    }

    return std::nullopt;
}

[[nodiscard]] Expected<std::vector<Step>, StepOrderError>
topologicalSort(const std::vector<Step>& steps,
                const Adjacency& adjacency,
                InDegree inDegree,
                const std::unordered_map<std::string, const Step*>& stepByName)
{
    std::vector<std::string> ready;
    ready.reserve(steps.size());
    for (const auto& [name, degree] : inDegree)
    {
        if (degree == 0)
        {
            ready.push_back(name);
        }
    }

    std::ranges::sort(ready);

    std::vector<Step> ordered;
    ordered.reserve(steps.size());

    while (!ready.empty())
    {
        const std::string Current = ready.front();
        ready.erase(ready.begin());
        ordered.push_back(*stepByName.at(Current));

        const auto Successors = adjacency.find(Current);
        if (Successors == adjacency.end())
        {
            continue;
        }

        for (const auto& successor : Successors->second)
        {
            auto& degree = inDegree.at(successor);
            if (degree == 0)
            {
                continue;
            }

            --degree;
            if (degree == 0)
            {
                ready.push_back(successor);
            }
        }

        std::ranges::sort(ready);
    }

    if (ordered.size() != steps.size())
    {
        StepOrderError error;
        error.kind = StepOrderErrorKind::Cycle;
        error.message = "cyclic dependency detected between steps in the same phase";
        return error;
    }

    return ordered;
}

}  // namespace

Expected<std::vector<Step>, StepOrderError> orderSteps(const std::vector<Step>& steps,
                                                       const std::vector<StepOrderHint>& hints,
                                                       const IGlobMatcher& matcher)
{
    if (steps.empty())
    {
        return std::vector<Step> {};
    }

    Adjacency adjacency;
    InDegree inDegree;
    std::unordered_map<std::string, const Step*> stepByName;

    if (const auto BuildError =
            buildDependencyGraph(steps, hints, matcher, adjacency, inDegree, stepByName))
    {
        return *BuildError;
    }

    return topologicalSort(steps, adjacency, std::move(inDegree), stepByName);
}

}  // namespace beez::core
