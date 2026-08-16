#!/usr/bin/env lua

local script_path = debug.getinfo(1, "S").source:gsub("^@", "")
local script_dir = script_path:match("^(.*)/[^/]+$")
local util_boot = dofile(script_dir .. "/../../packaging/reqpack/plugin/lib/util.lua")
local repo_root = util_boot.join_path(script_dir, "..", "..")
local util = dofile(util_boot.join_path(repo_root, "packaging/reqpack/plugin/lib/util.lua"))

local DEFAULT_OUTPUT_DIR = util.join_path(repo_root, "..", "beez-plugins", "beez-clang")
local LICENSE_PATH = util.join_path(repo_root, "LICENSE")
local GITHUB_REPO = "Coditary/beez-clang"
local ORGANIZATION = "coditary"
local PLUGIN_VERSION = "1.0.0"

local CLANG_PLUGINS = {
  "clang",
  "clang-format",
  "clang-tidy",
}

local function usage()
  io.stderr:write("Usage: lua scripts/ci/bootstrap_beez_clang_monorepo.lua [--output-dir <path>] [--init-git]\n")
end

local function parse_args(argv)
  local options = {
    output_dir = DEFAULT_OUTPUT_DIR,
    init_git = false,
  }
  local index = 1
  while index <= #argv do
    local arg = argv[index]
    if arg == "--output-dir" then
      options.output_dir = argv[index + 1]
      index = index + 2
    elseif arg == "--init-git" then
      options.init_git = true
      index = index + 1
    elseif arg == "-h" or arg == "--help" then
      usage()
      os.exit(0)
    end
  end
  return options
end

local function read_text(path)
  local file = io.open(path, "rb")
  if file == nil then
    return nil
  end
  local content = file:read("*a") or ""
  file:close()
  return content
end

local function write_text(path, content)
  util.mkdir_p(path:match("^(.*)/[^/]+$") or ".")
  local file, err = io.open(path, "wb")
  if file == nil then
    error("failed to write " .. path .. ": " .. tostring(err))
  end
  file:write(content)
  file:close()
end

local function copy_tree(source_dir, destination_dir)
  util.mkdir_p(destination_dir)
  local ok, output = util.run_shell(
    "cp -R "
      .. util.shell_quote(util.join_path(source_dir, "."))
      .. " "
      .. util.shell_quote(destination_dir)
  )
  if not ok then
    error("copy failed: " .. util.trim(output))
  end
end

local function read_plugin_description(source_dir)
  local content = read_text(util.join_path(source_dir, "beez_plugin.lua"))
  if content == nil then
    return ""
  end
  return content:match('description%s*=%s*"([^"]+)"') or ""
end

local function write_beez_package_json(path, plugin_name)
  write_text(
    path,
    table.concat({
      "{",
      '  "schemaVersion": 1,',
      string.format('  "organization": %q,', ORGANIZATION),
      string.format('  "name": %q,', plugin_name),
      string.format('  "version": %q,', PLUGIN_VERSION),
      '  "entry": "beez_plugin.lua"',
      "}",
      "",
    }, "\n")
  )
end

local function write_plugin_readme(path, plugin_name, description)
  local plugin_id = ORGANIZATION .. "/" .. plugin_name
  write_text(
    path,
    table.concat({
      "# " .. plugin_name,
      "",
      description,
      "",
      "Part of the [" .. GITHUB_REPO .. "](https://github.com/" .. GITHUB_REPO .. ") monorepo.",
      "",
      "## Install",
      "",
      "From the Beez plugin catalog:",
      "",
      "```bash",
      "rqp install beez " .. plugin_id .. "@" .. PLUGIN_VERSION,
      "```",
      "",
      "From this subdirectory via `build.lua`:",
      "",
      "```lua",
      "reqpack {",
      "  beez = {",
      "    {",
      '      name = "' .. plugin_id .. '",',
      '      version = "' .. PLUGIN_VERSION .. '",',
      '      source = "github:' .. GITHUB_REPO .. "#" .. plugin_name .. '",',
      "    },",
      "  },",
      "}",
      "```",
      "",
    }, "\n")
  )
end

local function write_root_readme(path)
  write_text(
    path,
    table.concat({
      "# beez-clang",
      "",
      "Monorepo of Clang-related Beez workflow plugins.",
      "",
      "## Plugins",
      "",
      "| Directory | Plugin ID |",
      "|-----------|-----------|",
      "| `clang/` | `coditary/clang` |",
      "| `clang-format/` | `coditary/clang-format` |",
      "| `clang-tidy/` | `coditary/clang-tidy` |",
      "",
      "## Layout",
      "",
      "```",
      "beez-clang/",
      "  clang/",
      "    beez.package.json",
      "    beez_plugin.lua",
      "    src/",
      "  clang-format/",
      "  clang-tidy/",
      "  releases/            # optional packaged .rqp + index.json (CI)",
      "```",
      "",
      "## Install",
      "",
      "Catalog install (after `beez-registry` points clang plugins here):",
      "",
      "```bash",
      "rqp install beez coditary/clang-format@1.0.0",
      "```",
      "",
      "Explicit monorepo path:",
      "",
      "```lua",
      'source = "github:' .. GITHUB_REPO .. '#clang-format"',
      "```",
      "",
      "## Bootstrap",
      "",
      "Regenerate from the Beez monorepo:",
      "",
      "```bash",
      "cd ../Beez",
      "lua scripts/ci/bootstrap_beez_clang_monorepo.lua --init-git",
      "```",
      "",
      "## Publish",
      "",
      "1. Push to `https://github.com/" .. GITHUB_REPO .. "`.",
      "2. Tag a release and attach per-plugin indexes under `clang-format/index.json`, etc.",
      "3. Run `lua scripts/ci/sync_plugin_catalog.lua` in Beez to refresh `beez-registry`.",
      "",
    }, "\n")
  )
end

local function init_git_repo(repo_dir)
  local exists = util.run_shell("test -d " .. util.shell_quote(util.join_path(repo_dir, ".git")))
  if exists then
    return
  end
  local ok, output = util.run_shell("git -C " .. util.shell_quote(repo_dir) .. " init -b main")
  if not ok then
    error("git init failed: " .. util.trim(output))
  end
end

local function main(argv)
  local options = parse_args(argv)
  local license = read_text(LICENSE_PATH)
  if license == nil then
    error("LICENSE not found")
  end

  util.mkdir_p(options.output_dir)
  write_root_readme(util.join_path(options.output_dir, "README.md"))
  write_text(
    util.join_path(options.output_dir, ".gitignore"),
    table.concat({ ".DS_Store", "dist/", "releases/", "*.rqp", "", }, "\n")
  )
  write_text(util.join_path(options.output_dir, "LICENSE"), license)

  for _, plugin_name in ipairs(CLANG_PLUGINS) do
    local source_dir = util.join_path(repo_root, "plugins", ORGANIZATION, plugin_name, PLUGIN_VERSION)
    local plugin_dir = util.join_path(options.output_dir, plugin_name)
    util.run_shell("rm -rf " .. util.shell_quote(plugin_dir))
    copy_tree(source_dir, plugin_dir)
    write_beez_package_json(util.join_path(plugin_dir, "beez.package.json"), plugin_name)
    write_plugin_readme(util.join_path(plugin_dir, "README.md"), plugin_name, read_plugin_description(source_dir))
    io.write("created " .. plugin_dir .. "\n")
  end

  if options.init_git then
    init_git_repo(options.output_dir)
  end

  io.write("wrote " .. options.output_dir .. "\n")
  return 0
end

local ok, err = pcall(main, { ... })
if not ok then
  io.stderr:write(tostring(err) .. "\n")
  os.exit(1)
end
