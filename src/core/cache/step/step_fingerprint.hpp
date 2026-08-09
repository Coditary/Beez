#pragma once

#include "beez/core/cache/fingerprint/content_hash.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace beez::core::step_cache_detail
{

[[nodiscard]] std::string stepExecutionIdentity(const Step& step);

[[nodiscard]] std::string configFingerprint(const StepConfigPtr& config);

[[nodiscard]] std::vector<std::string> artifactPatternsForInputs(const Step& step);

[[nodiscard]] std::string buildScriptFingerprint(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher);

[[nodiscard]] std::string stepCommandFingerprint(const Step& step,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher);

}  // namespace beez::core::step_cache_detail
