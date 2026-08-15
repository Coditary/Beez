#include "beez/plugin/lua/api/env/env.hpp"

#include "beez/core/env/env_file.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

class BeezEnvApi
{
  public:
    explicit BeezEnvApi(const core::Context& context) : envFilePath_(context.envFilePath()) {}

    [[nodiscard]] sol::object lookup(sol::this_state lua, const std::string& key) const
    {
        const auto Value = lookupValue(key);
        if (!Value.has_value())
        {
            return sol::lua_nil;
        }

        return sol::make_object(lua, *Value);
    }

    [[nodiscard]] sol::object lookupFirst(sol::this_state lua, sol::variadic_args keys) const
    {
        if (keys.size() == 0)
        {
            throw std::runtime_error("beez.env requires at least one key");
        }

        for (const sol::stack_proxy& key : keys)
        {
            if (!key.is<std::string>())
            {
                throw std::runtime_error("beez.env keys must be strings");
            }

            const auto Value = lookupValue(key.as<std::string>());
            if (Value.has_value())
            {
                return sol::make_object(lua, *Value);
            }
        }

        return sol::lua_nil;
    }

    [[nodiscard]] sol::object lookupOrDefault(sol::this_state lua, sol::variadic_args args) const
    {
        if (args.size() < 2)
        {
            throw std::runtime_error(
                "beez.env_or requires at least two arguments: one or more keys and a default");
        }

        for (std::size_t index = 0; index < args.size() - 1; ++index)
        {
            const auto& argument = args[static_cast<std::ptrdiff_t>(index)];
            if (!argument.is<std::string>())
            {
                throw std::runtime_error("beez.env_or keys must be strings");
            }

            const auto Value = lookupValue(argument.as<std::string>());
            if (Value.has_value())
            {
                return sol::make_object(lua, *Value);
            }
        }

        const sol::stack_proxy& defaultValue = args[static_cast<std::ptrdiff_t>(args.size() - 1)];
        if (!defaultValue.is<std::string>())
        {
            throw std::runtime_error("beez.env_or default must be a string");
        }

        return sol::make_object(lua, defaultValue.as<std::string>());
    }

  private:
    [[nodiscard]] std::optional<std::string> lookupValue(const std::string& key) const
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c) -- process env lookup for build DSL
        if (const char* processValue = std::getenv(key.c_str()))
        {
            return std::string(processValue);
        }

        if (!envFile_.has_value())
        {
            envFile_.emplace(envFilePath_);
        }

        return envFile_->lookup(key);
    }

    std::filesystem::path envFilePath_;
    mutable std::optional<core::EnvFile> envFile_;
};

}  // namespace

void bindEnvToTable(sol::table& beezTable, const core::Context& context)
{
    const auto Api = std::make_shared<BeezEnvApi>(context);
    beezTable["env"] = [Api](sol::this_state lua, sol::variadic_args keys)
    { return Api->lookupFirst(lua, keys); };
    beezTable["env_or"] = [Api](sol::this_state lua, sol::variadic_args args)
    { return Api->lookupOrDefault(lua, args); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
