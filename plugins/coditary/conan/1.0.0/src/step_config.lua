local defaults = require("src.defaults")

local M = {}

function M.graph_defaults()
    return {
        graph_rev = defaults.graph_rev,
        conanfile = defaults.conanfile,
        graph_json = defaults.graph_json,
        sbom_dir = defaults.sbom_dir,
        log_prefix_graph = defaults.log_prefix_graph,
    }
end

function M.lock_defaults()
    return {
        lock_rev = defaults.lock_rev,
        conanfile = defaults.conanfile,
        lockfile = defaults.lockfile,
        log_prefix_lock = defaults.log_prefix_lock,
    }
end

function M.sbom_defaults()
    return {
        sbom_rev = defaults.sbom_rev,
        conanfile = defaults.conanfile,
        graph_json = defaults.graph_json,
        cyclonedx_json = defaults.cyclonedx_json,
        sbom_dir = defaults.sbom_dir,
        converter_script = defaults.converter_script,
        log_prefix_sbom = defaults.log_prefix_sbom,
    }
end

function M.install_defaults()
    return {
        install_rev = defaults.install_rev,
        profile = "code",
        log_prefix_install = defaults.log_prefix_install,
    }
end

function M.configure_defaults(profile_name)
    return {
        profile = profile_name,
        configure_rev = defaults.configure_rev,
        log_prefix_configure = defaults.log_prefix_configure,
    }
end

function M.build_defaults(profile_name)
    return {
        profile = profile_name,
        build_rev = defaults.build_rev,
        log_prefix_build = defaults.log_prefix_build,
    }
end

return M
