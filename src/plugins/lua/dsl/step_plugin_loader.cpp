#include "beez/plugin/lua/dsl/step_plugin_loader.hpp"

#include "beez/core/plugin/installer.hpp"
#include "beez/core/registry/step_reference.hpp"
#include "beez/plugin/lua/dsl/plugin_loader.hpp"

namespace beez::plugin::lua
{

StepPluginEnsureResult ensureInstalledPluginForStepReference(const std::string& reference,
                                                             core::Registry& registry,
                                                             const core::Context& context)
{
    const auto [BaseReference, Version] = core::splitStepReferenceVersion(reference);
    (void)BaseReference;
    if (!Version.has_value())
    {
        return {.success = true};
    }

    const auto PluginIdentity = core::extractPluginIdentity(reference);
    if (!PluginIdentity.has_value())
    {
        return {.success = false,
                .message = "step reference '" + reference +
                           "' requires a plugin-qualified name when using @version"};
    }

    std::optional<std::string> organization = PluginIdentity->organization;
    if (!organization.has_value())
    {
        organization = core::resolveInstalledBeezPluginOrganization(PluginIdentity->plugin,
                                                                    *Version);
    }

    if (!organization.has_value())
    {
        return {.success = false,
                .message = "step reference '" + reference +
                           "' requires organization/plugin form to install versioned plugins "
                           "(for example coditary/" +
                           PluginIdentity->plugin + ":step@" + *Version + ')'};
    }

    if (registry.hasPluginVersionLoaded(*organization, PluginIdentity->plugin, *Version))
    {
        return {.success = true};
    }

    if (!core::resolveInstalledBeezPluginScript(organization, PluginIdentity->plugin, *Version)
             .hasValue())
    {
        const auto InstallResult = core::ensureBeezPluginInstalled(
            organization, PluginIdentity->plugin, *Version);
        if (!InstallResult.message.empty())
        {
            return {.success = false, .message = InstallResult.message};
        }
    }

    try
    {
        loadInstalledBeezPlugin(*organization, PluginIdentity->plugin, *Version, registry, context);
    }
    catch (const std::exception& error)
    {
        return {.success = false, .message = error.what()};
    }

    return {.success = true};
}

}  // namespace beez::plugin::lua
