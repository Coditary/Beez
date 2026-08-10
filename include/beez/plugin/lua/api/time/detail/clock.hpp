#pragma once

#include <cstdint>
#include <string>

namespace beez::plugin::lua::time_detail
{

[[nodiscard]] std::string nowMillisString();
[[nodiscard]] std::string uptimeMillisString();
[[nodiscard]] std::string iso8601UtcNow();

void sleepMillis(std::int64_t milliseconds);
void sleepSeconds(double seconds);

}  // namespace beez::plugin::lua::time_detail
