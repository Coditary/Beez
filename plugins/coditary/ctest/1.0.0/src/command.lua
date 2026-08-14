
local M = {}

local function report_parent_dir(marker)
    local parent = marker:match("^(.*)/[^/]+$")
    if parent == nil or parent == "" then
        return marker
    end
    return parent
end

local function ctest_lock(build_tree)
    return beez.char.quote(build_tree .. "/.ctest.lock")
end

local function ctest_timeout_arg(config)
    if config.ctest_timeout == nil then
        return ""
    end

    return " --timeout " .. tostring(config.ctest_timeout)
end

local function flock_wrap(build_tree, inner_cmd)
  return "flock " .. ctest_lock(build_tree) .. " bash -c " .. beez.char.quote(inner_cmd)
end

function M.ctest(config, root)
    local build_tree = root .. "/" .. config.build_tree
    local report_ok = root .. "/" .. config.report_marker
    local report_dir = root .. "/" .. report_parent_dir(config.report_marker)

    local inner = table.concat({
        "mkdir -p " .. beez.char.quote(report_dir),
        "&& cd " .. beez.char.quote(build_tree),
        "&& ctest " .. config.ctest_args .. ctest_timeout_arg(config) .. " --output-on-failure",
        "&& touch " .. beez.char.quote(report_ok),
    }, " ")

    return flock_wrap(build_tree, inner)
end

function M.ctest_tee(config, root)
    local build_tree = root .. "/" .. config.build_tree
    local report_dir = root .. "/" .. config.reports_dir .. "/" .. config.report_subdir
    local report_ok = report_dir .. "/" .. config.report_ok
    local tee_relative = "../../../" .. config.reports_dir .. "/" .. config.report_subdir .. "/" .. config.report_txt

    local inner = table.concat({
        "mkdir -p " .. beez.char.quote(report_dir),
        "&& cd " .. build_tree,
        " && ctest --output-on-failure" .. ctest_timeout_arg(config) .. " 2>&1 | tee " .. tee_relative,
        "&& touch " .. beez.char.quote(report_ok),
    }, " ")

    return flock_wrap(build_tree, inner)
end

function M.script(config, root)
    local script_path = root .. "/" .. config.script
    local build_tree = root .. "/" .. config.build_tree
    local inner = beez.char.quote(script_path) .. " build " .. beez.char.quote(config.reports_dir)
    return flock_wrap(build_tree, inner)
end

return M
