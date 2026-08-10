#pragma once

#include "beez/core/config/ui/types.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace beez::core
{

[[nodiscard]] std::string
formatProgressLine(const UiSettings& settings,
                   const logging::ExecutionProgress& progress,
                   bool cached = false,
                   std::optional<std::size_t> spinnerFrame = std::nullopt);

[[nodiscard]] std::string formatWorkerOutputPrefix(const UiSettings& settings,
                                                   std::uint64_t channelId,
                                                   std::string_view channelLabel);

}  // namespace beez::core
