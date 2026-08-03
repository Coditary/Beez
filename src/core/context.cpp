#include "beez/core/context.h"

#include <filesystem>
#include <utility>

namespace beez::core
{

Context::Context(std::filesystem::path projectRoot) : projectRoot_(std::move(projectRoot)) {}

std::filesystem::path Context::buildScriptPath() const
{
    return projectRoot_ / "build.lua";
}

}  // namespace beez::core
