#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace beez::cli
{

[[nodiscard]] int runInstallCompletion(const char* argv0);

[[nodiscard]] std::optional<std::string_view> dumpCompletionScript(const std::string& shell);

}  // namespace beez::cli
