#include "step_fingerprint.hpp"

#include "beez/core/cache/fingerprint/content_hash.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace beez::core::step_cache_detail
{

std::string stepExecutionIdentity(const Step& step)
{
    std::ostringstream stream;
    stream << step.name << '\0' << step.phase << '\0' << step.scope << '\0';
    if (step.shellRun.has_value())
    {
        stream << *step.shellRun;
    }
    else if (step.hasCallback())
    {
        stream << "<callback>";
    }
    return stream.str();
}

std::string configFingerprint(const StepConfigPtr& config)
{
    if (config == nullptr || config->empty())
    {
        return {};
    }
    return config->cacheFingerprint();
}

std::vector<std::string> artifactPatternsForInputs(const Step& step)
{
    std::vector<std::string> patterns = step.input;
    patterns.insert(patterns.end(), step.mutate.begin(), step.mutate.end());
    return patterns;
}

std::string buildScriptFingerprint(const Step& step,
                                   const std::filesystem::path& projectRoot,
                                   const IContentHasher& hasher)
{
    if (!step.hasCallback())
    {
        return {};
    }

    return hasher.hashFile(projectRoot / "build.lua");
}

std::string stepCommandFingerprint(const Step& step,
                                   const std::filesystem::path& projectRoot,
                                   const IContentHasher& hasher)
{
    if (step.shellRun.has_value())
    {
        return *step.shellRun;
    }

    if (step.hasCallback())
    {
        return std::string("<callback>:") + buildScriptFingerprint(step, projectRoot, hasher);
    }

    return {};
}

}  // namespace beez::core::step_cache_detail
