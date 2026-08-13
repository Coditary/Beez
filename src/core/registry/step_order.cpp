#include "beez/core/registry/step_order.hpp"

#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/util/expected.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
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

struct PatternRef
{
    const Step* step = nullptr;
    const std::string* pattern = nullptr;
};

using PrefixBuckets = std::unordered_map<std::string, std::vector<PatternRef>>;

[[nodiscard]] std::string globLiteralPrefix(const std::string& pattern)
{
    const auto Wildcard = std::ranges::find_if(
        pattern, [](const char Character) { return Character == '*' || Character == '?'; });
    return pattern.substr(0, static_cast<std::size_t>(std::distance(pattern.begin(), Wildcard)));
}

[[nodiscard]] bool literalPrefixesCompatible(const std::string& left, const std::string& right)
{
    if (left.empty() || right.empty())
    {
        return true;
    }

    if (left == right)
    {
        return true;
    }

    return left.starts_with(right) || right.starts_with(left);
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

[[nodiscard]] bool isReachableBefore(const std::vector<StepOrderHint>& hints,
                                     const std::string& before,
                                     const std::string& after)
{
    if (before == after)
    {
        return false;
    }

    if (hasHint(hints, before, after))
    {
        return true;
    }

    std::vector<std::string> queue = {before};
    std::set<std::string> visited;
    visited.insert(before);

    while (!queue.empty())
    {
        const std::string Current = queue.back();
        queue.pop_back();

        for (const auto& hint : hints)
        {
            if (hint.before != Current || visited.contains(hint.after))
            {
                continue;
            }

            if (hint.after == after)
            {
                return true;
            }

            visited.insert(hint.after);
            queue.push_back(hint.after);
        }
    }

    return false;
}

void collectPatternRefs(const Step& step,
                        const std::vector<std::string>& patterns,
                        std::vector<PatternRef>& refs)
{
    std::ranges::transform(patterns,
                           std::back_inserter(refs),
                           [&step](const std::string& pattern) -> PatternRef
                           { return PatternRef {.step = &step, .pattern = &pattern}; });
}

[[nodiscard]] PrefixBuckets bucketByLiteralPrefix(const std::vector<PatternRef>& refs)
{
    PrefixBuckets buckets;
    buckets.reserve(refs.size());
    for (const auto& ref : refs)
    {
        buckets[globLiteralPrefix(*ref.pattern)].push_back(ref);
    }
    return buckets;
}

void linkProducerBeforeConsumer(const std::vector<PatternRef>& producers,
                                const std::vector<PatternRef>& consumers,
                                const IGlobMatcher& matcher,
                                Adjacency& adjacency,
                                InDegree& inDegree)
{
    if (producers.empty() || consumers.empty())
    {
        return;
    }

    const PrefixBuckets ConsumerBuckets = bucketByLiteralPrefix(consumers);
    for (const auto& producer : producers)
    {
        const std::string ProducerPrefix = globLiteralPrefix(*producer.pattern);
        for (const auto& [consumerPrefix, consumerRefs] : ConsumerBuckets)
        {
            if (!literalPrefixesCompatible(ProducerPrefix, consumerPrefix))
            {
                continue;
            }

            for (const auto& consumer : consumerRefs)
            {
                if (producer.step == consumer.step)
                {
                    continue;
                }

                if (!matcher.patternsOverlap(*producer.pattern, *consumer.pattern))
                {
                    continue;
                }

                addEdge(producer.step->name, consumer.step->name, adjacency, inDegree);
            }
        }
    }
}

[[nodiscard]] std::optional<StepOrderError>
resolveMutateOverlap(const Step& step,
                     const Step& other,
                     const std::vector<StepOrderHint>& hints,
                     Adjacency& adjacency,
                     InDegree& inDegree);

[[nodiscard]] bool tryLinkMutatePair(const PatternRef& left,
                                     const PatternRef& right,
                                     const std::vector<StepOrderHint>& hints,
                                     const IGlobMatcher& matcher,
                                     Adjacency& adjacency,
                                     InDegree& inDegree,
                                     std::optional<StepOrderError>& error)
{
    if (left.step == right.step)
    {
        return false;
    }

    if (!matcher.patternsOverlap(*left.pattern, *right.pattern))
    {
        return false;
    }

    if (const auto Conflict =
            resolveMutateOverlap(*left.step, *right.step, hints, adjacency, inDegree))
    {
        error = Conflict;
        return true;
    }

    return false;
}

void linkMutateConflicts(const std::vector<PatternRef>& mutates,
                         const std::vector<StepOrderHint>& hints,
                         const IGlobMatcher& matcher,
                         Adjacency& adjacency,
                         InDegree& inDegree,
                         std::optional<StepOrderError>& error)
{
    if (mutates.size() < 2)
    {
        return;
    }

    const PrefixBuckets MutateBuckets = bucketByLiteralPrefix(mutates);
    for (auto left = MutateBuckets.begin(); left != MutateBuckets.end(); ++left)
    {
        for (auto right = left; right != MutateBuckets.end(); ++right)
        {
            if (!literalPrefixesCompatible(left->first, right->first))
            {
                continue;
            }

            const auto& leftRefs = left->second;
            const auto& rightRefs = right->second;
            for (std::size_t leftIndex = 0; leftIndex < leftRefs.size(); ++leftIndex)
            {
                const std::size_t RightStart = (left == right) ? leftIndex + 1 : 0;
                for (std::size_t rightIndex = RightStart; rightIndex < rightRefs.size();
                     ++rightIndex)
                {
                    if (tryLinkMutatePair(leftRefs.at(leftIndex),
                                          rightRefs.at(rightIndex),
                                          hints,
                                          matcher,
                                          adjacency,
                                          inDegree,
                                          error))
                    {
                        return;
                    }
                }
            }
        }
    }
}

[[nodiscard]] std::optional<StepOrderError>
resolveMutateOverlap(const Step& step,
                     const Step& other,
                     const std::vector<StepOrderHint>& hints,
                     Adjacency& adjacency,
                     InDegree& inDegree)
{
    const bool StepBeforeOther = isReachableBefore(hints, step.name, other.name);
    const bool OtherBeforeStep = isReachableBefore(hints, other.name, step.name);

    if (StepBeforeOther && OtherBeforeStep)
    {
        StepOrderError mutateError;
        mutateError.kind = StepOrderErrorKind::MutateConflict;
        mutateError.message = "conflicting order hints between mutate steps '" + step.name +
                              "' and '" + other.name + "'";
        return mutateError;
    }

    if (!StepBeforeOther && !OtherBeforeStep)
    {
        StepOrderError mutateError;
        mutateError.kind = StepOrderErrorKind::MutateConflict;
        mutateError.message = "cannot resolve mutate conflict between '" + step.name + "' and '" +
                              other.name + "'; add order(\"" + step.name + "\", \"" + other.name +
                              "\") or the reverse";
        return mutateError;
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

[[nodiscard]] bool stepHasArtifacts(const Step& step)
{
    return !step.input.empty() || !step.output.empty() || !step.mutate.empty();
}

[[nodiscard]] std::optional<StepOrderError>
buildDependencyGraph(const std::vector<Step>& steps,
                     const std::vector<StepOrderHint>& hints,
                     const IGlobMatcher& matcher,
                     Adjacency& adjacency,
                     InDegree& inDegree,
                     std::unordered_map<std::string, const Step*>& stepByName)
{
    std::vector<PatternRef> outputs;
    std::vector<PatternRef> inputs;
    std::vector<PatternRef> mutates;

    for (const auto& step : steps)
    {
        stepByName.emplace(step.name, &step);
        inDegree.emplace(step.name, 0);
        collectPatternRefs(step, step.output, outputs);
        collectPatternRefs(step, step.input, inputs);
        collectPatternRefs(step, step.mutate, mutates);
    }

    linkProducerBeforeConsumer(outputs, inputs, matcher, adjacency, inDegree);
    linkProducerBeforeConsumer(mutates, inputs, matcher, adjacency, inDegree);
    linkProducerBeforeConsumer(outputs, mutates, matcher, adjacency, inDegree);
    linkProducerBeforeConsumer(mutates, outputs, matcher, adjacency, inDegree);

    std::optional<StepOrderError> mutateError;
    linkMutateConflicts(mutates, hints, matcher, adjacency, inDegree, mutateError);
    if (mutateError.has_value())
    {
        return mutateError;
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
    std::set<std::string> ready;
    for (const auto& [name, degree] : inDegree)
    {
        if (degree == 0)
        {
            ready.insert(name);
        }
    }

    std::vector<Step> ordered;
    ordered.reserve(steps.size());

    while (!ready.empty())
    {
        const std::string Current = *ready.begin();
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
                ready.insert(successor);
            }
        }
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

[[nodiscard]] Expected<std::vector<std::vector<Step>>, StepOrderError>
topologicalLevels(const std::vector<Step>& steps,
                  const Adjacency& adjacency,
                  InDegree inDegree,
                  const std::unordered_map<std::string, const Step*>& stepByName)
{
    std::set<std::string> ready;
    for (const auto& [name, degree] : inDegree)
    {
        if (degree == 0)
        {
            ready.insert(name);
        }
    }

    std::vector<std::vector<Step>> levels;
    std::size_t processed = 0;

    while (!ready.empty())
    {
        std::vector<Step> level;
        level.reserve(ready.size());
        std::ranges::transform(ready,
                               std::back_inserter(level),
                               [&stepByName](const std::string& name) -> Step
                               { return *stepByName.at(name); });
        levels.push_back(std::move(level));
        processed += ready.size();

        std::set<std::string> nextReady;
        for (const auto& name : ready)
        {
            const auto Successors = adjacency.find(name);
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
                    nextReady.insert(successor);
                }
            }
        }

        ready = std::move(nextReady);
    }

    if (processed != steps.size())
    {
        StepOrderError error;
        error.kind = StepOrderErrorKind::Cycle;
        error.message = "cyclic dependency detected between steps in the same phase";
        return error;
    }

    return levels;
}

[[nodiscard]] Expected<std::vector<Step>, StepOrderError>
sortStepsAlphabetically(std::vector<Step> steps)
{
    std::ranges::sort(steps,
                      [](const Step& left, const Step& right) { return left.name < right.name; });
    return steps;
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

    const bool HasArtifacts = std::ranges::any_of(steps, stepHasArtifacts);
    if (!HasArtifacts && hints.empty())
    {
        return sortStepsAlphabetically(steps);
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

Expected<std::vector<std::vector<Step>>, StepOrderError>
orderStepsInLevels(const std::vector<Step>& steps,
                   const std::vector<StepOrderHint>& hints,
                   const IGlobMatcher& matcher)
{
    if (steps.empty())
    {
        return std::vector<std::vector<Step>> {};
    }

    const bool HasArtifacts = std::ranges::any_of(steps, stepHasArtifacts);
    if (!HasArtifacts && hints.empty())
    {
        const auto Sorted = sortStepsAlphabetically(steps);
        if (!Sorted.hasValue())
        {
            return Sorted.error();
        }

        return std::vector<std::vector<Step>> {Sorted.value()};
    }

    Adjacency adjacency;
    InDegree inDegree;
    std::unordered_map<std::string, const Step*> stepByName;

    if (const auto BuildError =
            buildDependencyGraph(steps, hints, matcher, adjacency, inDegree, stepByName))
    {
        return *BuildError;
    }

    return topologicalLevels(steps, adjacency, std::move(inDegree), stepByName);
}

std::vector<std::vector<Step>> isolateCallbackStepsInLevels(std::vector<std::vector<Step>> levels)
{
    std::vector<std::vector<Step>> isolated;
    isolated.reserve(levels.size());

    for (auto& level : levels)
    {
        if (level.size() <= 1U)
        {
            isolated.push_back(std::move(level));
            continue;
        }

        const std::size_t CallbackCount =
            std::ranges::count_if(level, [](const Step& step) { return step.hasCallback(); });
        if (CallbackCount <= 1U)
        {
            isolated.push_back(std::move(level));
            continue;
        }

        std::vector<Step> shellSteps;
        std::vector<Step> callbackSteps;
        shellSteps.reserve(level.size());
        callbackSteps.reserve(level.size());

        for (auto& step : level)
        {
            if (step.hasCallback())
            {
                callbackSteps.push_back(std::move(step));
            }
            else
            {
                shellSteps.push_back(std::move(step));
            }
        }

        if (!shellSteps.empty())
        {
            isolated.push_back(std::move(shellSteps));
        }

        std::ranges::transform(callbackSteps,
                               std::back_inserter(isolated),
                               [](Step callbackStep) -> std::vector<Step>
                               { return std::vector<Step> {std::move(callbackStep)}; });
    }

    return isolated;
}

}  // namespace beez::core
