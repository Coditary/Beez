#include "beez/plugin/lua/api/env.hpp"

#include "beez/core/env/env_file.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
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

    sol::object lookup(sol::this_state lua, const std::string& key) const
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c) -- process env lookup for build DSL
        if (const char* processValue = std::getenv(key.c_str()))
        {
            return sol::make_object(lua, std::string(processValue));
        }

        if (!envFile_.has_value())
        {
            envFile_.emplace(envFilePath_);
        }

        const auto Value = envFile_->lookup(key);
        if (!Value.has_value())
        {
            return sol::lua_nil;
        }

        return sol::make_object(lua, *Value);
    }

  private:
    std::filesystem::path envFilePath_;
    mutable std::optional<core::EnvFile> envFile_;
};

}  // namespace

void bindEnvToTable(sol::table& beezTable, const core::Context& context)
{
    const auto Api = std::make_shared<BeezEnvApi>(context);
    beezTable["env"] = [Api](sol::this_state lua, const std::string& key)
    { return Api->lookup(lua, key); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
