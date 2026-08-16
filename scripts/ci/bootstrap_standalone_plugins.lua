#!/usr/bin/env lua

local script_path = debug.getinfo(1, "S").source:gsub("^@", "")
local script_dir = script_path:match("^(.*)/[^/]+$")
local util_boot = dofile(script_dir .. "/../../packaging/reqpack/plugin/lib/util.lua")
local repo_root = util_boot.join_path(script_dir, "..", "..")
local util = dofile(util_boot.join_path(repo_root, "packaging/reqpack/plugin/lib/util.lua"))

local DEFAULT_OUTPUT_DIR = util.join_path(repo_root, "..", "beez-plugins")
local AUTHOR = "Leonard Ramminger"
local EMAIL = "Matographo@gmail.com"
local ORGANIZATION = "coditary"
local GITHUB_ORG = "Coditary"
local LICENSE_PATH = util.join_path(repo_root, "LICENSE")
local EXCLUDED_PLUGINS = {
  ["clang"] = true,
  ["clang-format"] = true,
  ["clang-tidy"] = true,
}

local function usage()
  io.stderr:write(
    "Usage: lua scripts/ci/bootstrap_standalone_plugins.lua [--output-dir <path>] [--init-git]\n"
  )
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
    else
      error("unknown argument: " .. tostring(arg))
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

local function discover_plugins()
  local plugins_root = util.join_path(repo_root, "plugins", ORGANIZATION)
  local plugins = {}
  local ok, output = util.run_shell(
    "find " .. util.shell_quote(plugins_root) .. " -mindepth 2 -maxdepth 2 -type d | sort"
  )
  if not ok then
    return plugins
  end

  for line in (output .. "\n"):gmatch("(.-)\n") do
    local version_dir = util.trim(line)
    if version_dir ~= "" then
      local version = version_dir:match("/([^/]+)$")
      local plugin_dir = version_dir:match("^(.*)/[^/]+$")
      local name = plugin_dir:match("/([^/]+)$")
      if name ~= nil and version ~= nil and not EXCLUDED_PLUGINS[name] then
        plugins[#plugins + 1] = {
          name = name,
          version = version,
          source_dir = version_dir,
        }
      end
    end
  end
  return plugins
end

local function read_plugin_description(source_dir)
  local content = read_text(util.join_path(source_dir, "beez_plugin.lua"))
  if content == nil then
    return ""
  end
  return content:match('description%s*=%s*"([^"]+)"') or ""
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
    error("copy failed for " .. source_dir .. ": " .. util.trim(output))
  end
end

local function json_string(value)
  return string.format("%q", value)
end

local function write_beez_package_json(path, plugin)
  local payload = {
    "{",
    '  "schemaVersion": 1,',
    string.format('  "organization": %s,', json_string(ORGANIZATION)),
    string.format('  "name": %s,', json_string(plugin.name)),
    string.format('  "version": %s,', json_string(plugin.version)),
    '  "entry": "beez_plugin.lua"',
    "}",
    "",
  }
  write_text(path, table.concat(payload, "\n"))
end

local function write_gitignore(path)
  write_text(path, table.concat({
    ".DS_Store",
    "Thumbs.db",
    "*.swp",
    "*.swo",
    "*~",
    ".vscode/",
    ".idea/",
    "",
  }, "\n"))
end

local function write_readme(path, plugin)
  local repo_name = "beez-" .. plugin.name
  local plugin_id = ORGANIZATION .. "/" .. plugin.name
  local content = table.concat({
    "# " .. plugin.name,
    "",
    plugin.description,
    "",
    "Standalone Beez workflow plugin published by **" .. ORGANIZATION .. "**.",
    "",
    "## Install",
    "",
    "From the Beez plugin catalog (after release packaging):",
    "",
    "```bash",
    "rqp install beez " .. plugin_id .. "@" .. plugin.version,
    "```",
    "",
    "From this repository (git source):",
    "",
    "```bash",
    "# build.lua",
    'reqpack { beez = { { name = "' .. plugin_id .. '", version = "' .. plugin.version .. '", source = "github:' .. GITHUB_ORG .. "/" .. repo_name .. '" } } }',
    "```",
    "",
    "```bash",
    "beez --install",
    "```",
    "",
    "Local development:",
    "",
    "```bash",
    'reqpack { beez = { { name = "' .. plugin_id .. '", path = ".", version = "' .. plugin.version .. '" } } }',
    "```",
    "",
    "## Layout",
    "",
    "```",
    "beez.package.json",
    "beez_plugin.lua",
    "src/",
    "scripts/   # optional helper scripts",
    "```",
    "",
    "## Package",
    "",
    "Build a `.rqp` from the Beez monorepo packaging scripts:",
    "",
    "```bash",
    "lua scripts/ci/package_beez_plugin.lua \\",
    "  --organization " .. ORGANIZATION .. " \\",
    "  --name " .. plugin.name .. " \\",
    "  --version " .. plugin.version .. " \\",
    "  --source-dir . \\",
    "  --platform linux \\",
    "  --arch x86_64 \\",
    "  --output-dir dist",
    "```",
    "",
    "## Metadata",
    "",
    "| Field | Value |",
    "|-------|-------|",
    "| Plugin ID | `" .. plugin_id .. "` |",
    "| Version | `" .. plugin.version .. "` |",
    "| Suggested repo | `" .. GITHUB_ORG .. "/" .. repo_name .. "` |",
    "| Maintainer | " .. AUTHOR .. " <" .. EMAIL .. "> |",
    "",
    "## License",
    "",
    "Apache License 2.0. See [LICENSE](LICENSE).",
    "",
  }, "\n")
  write_text(path, content)
end

local function init_git_repo(plugin_dir)
  local exists = util.run_shell("test -d " .. util.shell_quote(util.join_path(plugin_dir, ".git")))
  if exists then
    return
  end
  local ok, output = util.run_shell("git -C " .. util.shell_quote(plugin_dir) .. " init -b main")
  if not ok then
    error("git init failed for " .. plugin_dir .. ": " .. util.trim(output))
  end
end

local function main(argv)
  local options = parse_args(argv)
  local license = read_text(LICENSE_PATH)
  if license == nil then
    error("LICENSE not found at " .. LICENSE_PATH)
  end

  util.mkdir_p(options.output_dir)
  local plugins = discover_plugins()
  if #plugins == 0 then
    error("no plugins discovered")
  end

  local manifest_lines = {
    "# Beez Plugins",
    "",
    "Standalone Beez workflow plugins extracted from the Coditary Beez monorepo.",
    "",
    "## Clang monorepo",
    "",
    "Clang plugins live together in [`beez-clang/`](beez-clang/) (`Coditary/beez-clang` on GitHub).",
    "Refresh with `lua scripts/ci/bootstrap_beez_clang_monorepo.lua --init-git` in the Beez repo.",
    "",
    "## Standalone plugins",
    "",
    "Each other subdirectory is intended to become its own Git repository under `"
      .. GITHUB_ORG
      .. "/beez-<name>`.",
    "",
    "| Directory | Plugin ID | Suggested GitHub repo |",
    "|-----------|-----------|------------------------|",
  }

  for _, plugin in ipairs(plugins) do
    plugin.description = read_plugin_description(plugin.source_dir)
    local plugin_dir = util.join_path(options.output_dir, plugin.name)
    util.run_shell("rm -rf " .. util.shell_quote(plugin_dir))
    copy_tree(plugin.source_dir, plugin_dir)
    write_beez_package_json(util.join_path(plugin_dir, "beez.package.json"), plugin)
    write_readme(util.join_path(plugin_dir, "README.md"), plugin)
    write_gitignore(util.join_path(plugin_dir, ".gitignore"))
    write_text(util.join_path(plugin_dir, "LICENSE"), license)

    if options.init_git then
      init_git_repo(plugin_dir)
    end

    local repo_name = GITHUB_ORG .. "/beez-" .. plugin.name
    manifest_lines[#manifest_lines + 1] = string.format(
      "| `%s/` | `%s/%s` | `%s` |",
      plugin.name,
      ORGANIZATION,
      plugin.name,
      repo_name
    )
    io.write("created " .. plugin_dir .. "\n")
  end

  manifest_lines[#manifest_lines + 1] = ""
  manifest_lines[#manifest_lines + 1] = "## Bootstrap"
  manifest_lines[#manifest_lines + 1] = ""
  manifest_lines[#manifest_lines + 1] = "Regenerate from Beez:"
  manifest_lines[#manifest_lines + 1] = ""
  manifest_lines[#manifest_lines + 1] = "```bash"
  manifest_lines[#manifest_lines + 1] = "cd ../Beez"
  manifest_lines[#manifest_lines + 1] = "lua scripts/ci/bootstrap_beez_clang_monorepo.lua --init-git"
  manifest_lines[#manifest_lines + 1] = "lua scripts/ci/bootstrap_standalone_plugins.lua --init-git"
  manifest_lines[#manifest_lines + 1] = "lua scripts/ci/sync_plugin_catalog.lua"
  manifest_lines[#manifest_lines + 1] = "```"
  manifest_lines[#manifest_lines + 1] = ""
  manifest_lines[#manifest_lines + 1] = "## Publish"
  manifest_lines[#manifest_lines + 1] = ""
  manifest_lines[#manifest_lines + 1] = "1. Create the GitHub repository (empty)."
  manifest_lines[#manifest_lines + 1] = "2. Commit and push each plugin directory."
  manifest_lines[#manifest_lines + 1] = "3. Add or update the entry in `beez-registry/plugins.json`."
  manifest_lines[#manifest_lines + 1] = ""

  write_text(util.join_path(options.output_dir, "README.md"), table.concat(manifest_lines, "\n"))
  io.write("wrote " .. util.join_path(options.output_dir, "README.md") .. " (" .. tostring(#plugins) .. " plugins)\n")
  return 0
end

local ok, err = pcall(main, { ... })
if not ok then
  io.stderr:write(tostring(err) .. "\n")
  os.exit(1)
end
