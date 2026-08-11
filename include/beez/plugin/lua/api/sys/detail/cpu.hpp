#pragma once

#include <cstddef>

namespace beez::plugin::lua::sys_detail
{

[[nodiscard]] std::size_t cpuCoreCount();
[[nodiscard]] std::size_t cpuThreadCount();

}  // namespace beez::plugin::lua::sys_detail
