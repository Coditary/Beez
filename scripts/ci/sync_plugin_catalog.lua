#!/usr/bin/env lua

local script_path = debug.getinfo(1, "S").source:gsub("^@", "")
local script_dir = script_path:match("^(.*)/[^/]+$")
local util_boot = dofile(script_dir .. "/../../packaging/reqpack/plugin/lib/util.lua")
local repo_root = util_boot.join_path(script_dir, "..", "..")
local util = dofile(util_boot.join_path(repo_root, "packaging/reqpack/plugin/lib/util.lua"))

local DEFAULT_REGISTRY_DIR = util.join_path(repo_root, "..", "beez-registry")
local DEFAULT_INDEX_URL = "https://github.com/Coditary/Beez/releases/latest/download/plugins/index.json"
local CLANG_MONOREPO_URL = "github:Coditary/beez-clang"
local CLANG_MONOREPO_PLUGINS = {
  ["clang"] = true,
  ["clang-format"] = true,
  ["clang-tidy"] = true,
}

local GIT_STANDALONE_PLUGINS = {
  ["conan"] = true,
  ["coverage"] = true,
  ["cppcheck"] = true,
  ["ctest"] = true,
  ["cyclonedx"] = true,
  ["fuzzer"] = true,
  ["osv-audit"] = true,
  ["pipeline"] = true,
}

local function standalone_git_repo_url(plugin_name)
  return "github:Coditary/beez-" .. plugin_name
end

local TAGS = {
  ["clang"] = { "build", "clang", "coditary" },
  ["clang-format"] = { "format", "clang", "coditary" },
  ["clang-tidy"] = { "lint", "clang", "coditary" },
  ["conan"] = { "conan", "dependencies", "coditary" },
  ["cppcheck"] = { "lint", "static-analysis", "coditary" },
  ["coverage"] = { "coverage", "test", "coditary" },
  ["ctest"] = { "test", "ctest", "coditary" },
  ["cyclonedx"] = { "sbom", "security", "coditary" },
  ["fuzzer"] = { "fuzz", "security", "coditary" },
  ["osv-audit"] = { "security", "audit", "coditary" },
  ["pipeline"] = { "workflow", "pipeline", "coditary" },
}

local function usage()
  io.stderr:write(
    "Usage: lua scripts/ci/sync_plugin_catalog.lua [--registry-dir <path>] [--index-url <url>]\n"
  )
end

local function parse_args(argv)
  local options = {
    registry_dir = DEFAULT_REGISTRY_DIR,
    index_url = DEFAULT_INDEX_URL,
  }
  local index = 1
  while index <= #argv do
    local arg = argv[index]
    if arg == "--registry-dir" then
      options.registry_dir = argv[index + 1]
      index = index + 2
    elseif arg == "--index-url" then
      options.index_url = argv[index + 1]
      index = index + 2
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

local function discover_plugins(organization)
  local plugins_root = util.join_path(repo_root, "plugins", organization)
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
      if name ~= nil and version ~= nil then
        plugins[#plugins + 1] = {
          organization = organization,
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

local function json_string(value)
  return string.format("%q", value):gsub("\\\n", "\\n")
end

local function write_catalog(path, plugins, index_url)
  local lines = {
    "{",
    '  "schemaVersion": 1,',
    '  "plugins": [',
  }

  for plugin_index, plugin in ipairs(plugins) do
    local tags = TAGS[plugin.name] or { plugin.name, plugin.organization }
    local description = read_plugin_description(plugin.source_dir)
    lines[#lines + 1] = "    {"
    lines[#lines + 1] = string.format('      "id": %s,', json_string(plugin.organization .. "/" .. plugin.name))
    lines[#lines + 1] = string.format('      "version": %s,', json_string(plugin.version))
    lines[#lines + 1] = string.format('      "description": %s,', json_string(description))
    lines[#lines + 1] = '      "tags": ['
    for tag_index, tag in ipairs(tags) do
      local suffix = tag_index == #tags and "" or ","
      lines[#lines + 1] = string.format('        %s%s', json_string(tag), suffix)
    end
    lines[#lines + 1] = "      ],"
    lines[#lines + 1] = '      "source": {'
    if CLANG_MONOREPO_PLUGINS[plugin.name] then
      lines[#lines + 1] = '        "type": "git",'
      lines[#lines + 1] = string.format('        "url": %s,', json_string(CLANG_MONOREPO_URL))
      lines[#lines + 1] = string.format('        "path": %s,', json_string(plugin.name))
      lines[#lines + 1] = string.format('        "ref": %s', json_string(plugin.version))
    elseif GIT_STANDALONE_PLUGINS[plugin.name] then
      lines[#lines + 1] = '        "type": "git",'
      lines[#lines + 1] = string.format('        "url": %s,', json_string(standalone_git_repo_url(plugin.name)))
      lines[#lines + 1] = string.format('        "ref": %s', json_string(plugin.version))
    else
      lines[#lines + 1] = '        "type": "index",'
      lines[#lines + 1] = string.format('        "url": %s', json_string(index_url))
    end
    lines[#lines + 1] = "      }"
    if plugin_index == #plugins then
      lines[#lines + 1] = "    }"
    else
      lines[#lines + 1] = "    },"
    end
  end

  lines[#lines + 1] = "  ]"
  lines[#lines + 1] = "}"
  lines[#lines + 1] = ""

  local file, err = io.open(path, "wb")
  if file == nil then
    error("failed to write " .. path .. ": " .. tostring(err))
  end
  file:write(table.concat(lines, "\n"))
  file:close()
end

local function main(argv)
  local options = parse_args(argv)
  local plugins = discover_plugins("coditary")
  if #plugins == 0 then
    error("no plugins discovered under plugins/coditary")
  end

  table.sort(plugins, function(left, right)
    return left.name < right.name
  end)

  util.mkdir_p(options.registry_dir)
  local output_path = util.join_path(options.registry_dir, "plugins.json")
  write_catalog(output_path, plugins, options.index_url)
  io.write("wrote " .. output_path .. " (" .. tostring(#plugins) .. " plugins)\n")
  return 0
end

local ok, err = pcall(main, { ... })
if not ok then
  io.stderr:write(tostring(err) .. "\n")
  os.exit(1)
end
