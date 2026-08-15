#pragma once

namespace beez::core
{
class Registry;
}  // namespace beez::core

namespace beez::plugin::lua
{

void validateTaskInvocations(const core::Registry& registry);

}  // namespace beez::plugin::lua
