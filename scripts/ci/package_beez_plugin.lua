#!/usr/bin/env lua

local script_path = debug.getinfo(1, "S").source:gsub("^@", "")
local script_dir = script_path:match("^(.*)/[^/]+$")
local util_boot = dofile(script_dir .. "/../../packaging/reqpack/plugin/lib/util.lua")
local repo_root = util_boot.join_path(script_dir, "..", "..")

local util = dofile(repo_root .. "/packaging/reqpack/plugin/lib/util.lua")
local pack = dofile(repo_root .. "/packaging/reqpack/plugin/lib/pack.lua")

local function usage()
  io.stderr:write([[
Usage: lua scripts/ci/package_beez_plugin.lua \
  --organization <org> \
  --name <plugin> \
  --version <version> \
  --source-dir <path> \
  --platform <linux|macos> \
  --arch <x86_64|aarch64> \
  [--output-dir dist] \
  [--source-url <url>] \
  [--homepage <url>]

]])
end

local function parse_args(argv)
  local options = {
    output_dir = "dist",
  }

  local index = 1
  while index <= #argv do
    local arg = argv[index]
    if arg == "--organization" then
      options.organization = argv[index + 1]
      index = index + 2
    elseif arg == "--name" then
      options.name = argv[index + 1]
      index = index + 2
    elseif arg == "--version" then
      options.version = argv[index + 1]
      index = index + 2
    elseif arg == "--source-dir" then
      options.source_dir = argv[index + 1]
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
    elseif arg == "--source-url" then
      options.source_url = argv[index + 1]
      index = index + 2
    elseif arg == "--homepage" then
      options.homepage = argv[index + 1]
      index = index + 2
    elseif arg == "-h" or arg == "--help" then
      usage()
      os.exit(0)
    else
      error("unknown argument: " .. tostring(arg))
    end
  end

  for _, field in ipairs({ "organization", "name", "version", "source_dir", "platform", "arch" }) do
    if options[field] == nil or util.trim(options[field]) == "" then
      usage()
      error("missing required argument: --" .. field:gsub("_", "-"))
    end
  end

  options.source_dir = util.join_path(repo_root, options.source_dir)
  options.output_dir = util.join_path(repo_root, options.output_dir)
  options.template_root = util.join_path(repo_root, "packaging/reqpack/plugin")
  return options
end

local function main(argv)
  local options = parse_args(argv)
  local archive_path = pack.package_plugin(options)
  print(archive_path)
  return 0
end

local ok, err = pcall(main, { ... })
if not ok then
  io.stderr:write(tostring(err) .. "\n")
  os.exit(1)
end
