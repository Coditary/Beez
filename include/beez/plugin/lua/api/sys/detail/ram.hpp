#pragma once

#include <cstdint>

namespace beez::plugin::lua::sys_detail
{

[[nodiscard]] std::uint64_t ramTotalBytes();
[[nodiscard]] std::uint64_t ramFreeBytes();

}  // namespace beez::plugin::lua::sys_detail
