#!/usr/bin/env lua

local script_path = debug.getinfo(1, "S").source:gsub("^@", "")
local script_dir = script_path:match("^(.*)/[^/]+$")
local util_boot = dofile(script_dir .. "/../../packaging/reqpack/plugin/lib/util.lua")
local repo_root = util_boot.join_path(script_dir, "..", "..")
local pack = dofile(util_boot.join_path(repo_root, "packaging/reqpack/plugin/lib/pack.lua"))

local function usage()
  io.stderr:write(
    "Usage: lua scripts/ci/package_all_plugins.lua --version <version> --platform <linux|macos> --arch <arch> [--output-dir dist/plugins]\n"
  )
end

local function parse_args(argv)
  local options = {
    output_dir = "dist/plugins",
  }
  local index = 1
  while index <= #argv do
    local arg = argv[index]
    if arg == "--version" then
      options.version = argv[index + 1]
      index = index + 2
    elseif arg == "--platform" then
      options.platform = argv[index + 1]
      index = index + 2
    elseif arg == "--arch" then
      options.arch = argv[index + 1]
      index = index + 2
    elseif arg == "--output-dir" then
      options.output_dir = argv[index + 1]
      index = index + 2
    elseif arg == "-h" or arg == "--help" then
      usage()
      os.exit(0)
    else
      error("unknown argument: " .. tostring(arg))
    end
  end

  for _, field in ipairs({ "version", "platform", "arch" }) do
    if options[field] == nil or util_boot.trim(options[field]) == "" then
      usage()
      error("missing required argument: --" .. field)
    end
  end

  options.output_dir = util_boot.join_path(repo_root, options.output_dir)
  options.template_root = util_boot.join_path(repo_root, "packaging/reqpack/plugin")
  options.source_url = "https://github.com/Coditary/Beez"
  return options
end

local function discover_plugins()
  local plugins_root = util_boot.join_path(repo_root, "plugins/coditary")
  local plugins = {}
  local ok, output = util_boot.run_shell("find " .. util_boot.shell_quote(plugins_root) .. " -mindepth 2 -maxdepth 2 -type d | sort")
  if not ok then
    return plugins
  end

  for line in (output .. "\n"):gmatch("(.-)\n") do
    local version_dir = util_boot.trim(line)
    if version_dir ~= "" then
      local version = version_dir:match("/([^/]+)$")
      local plugin_dir = version_dir:match("^(.*)/[^/]+$")
      local name = plugin_dir:match("/([^/]+)$")
      if name ~= nil and version ~= nil then
        plugins[#plugins + 1] = {
          organization = "coditary",
          name = name,
          version = version,
          source_dir = version_dir,
        }
      end
    end
  end
  return plugins
end

local function main(argv)
  local options = parse_args(argv)
  util_boot.mkdir_p(options.output_dir)

  for _, plugin in ipairs(discover_plugins()) do
    pack.package_plugin({
      organization = plugin.organization,
      name = plugin.name,
      version = options.version ~= nil and util_boot.normalized_version(options.version) or plugin.version,
      source_dir = plugin.source_dir,
      platform = options.platform,
      arch = options.arch,
      output_dir = options.output_dir,
      template_root = options.template_root,
      source_url = options.source_url,
    })
  end

  return 0
end

local ok, err = pcall(main, { ... })
if not ok then
  io.stderr:write(tostring(err) .. "\n")
  os.exit(1)
end
