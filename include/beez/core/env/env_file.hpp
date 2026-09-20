#pragma once

#include <coditary/parser/env.hpp>

#include <optional>
#include <string>

namespace beez::core
{

using EnvFile = coditary::parser::EnvFile;

[[nodiscard]] inline std::optional<std::string> readProcessEnvironment(const std::string& key)
{
    return coditary::parser::read_process_environment(key);
}

}  // namespace beez::core
