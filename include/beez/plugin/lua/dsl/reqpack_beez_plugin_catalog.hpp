#pragma once

#include "beez/plugin/lua/dsl/plugin_loader.hpp"

#include <optional>
#include <string>
#include <vector>

namespace beez::plugin::lua
{

class ReqpackBeezPluginCatalog
{
  public:
    void set(const std::vector<BeezPluginRef>& plugins);
    void add(BeezPluginRef plugin);

    [[nodiscard]] bool empty() const
    {
        return plugins_.empty();
    }

    [[nodiscard]] std::optional<BeezPluginRef> find(const std::string& organization,
                                                    const std::string& plugin) const;

    [[nodiscard]] const std::vector<BeezPluginRef>& plugins() const
    {
        return plugins_;
    }

  private:
    std::vector<BeezPluginRef> plugins_;
};

}  // namespace beez::plugin::lua
