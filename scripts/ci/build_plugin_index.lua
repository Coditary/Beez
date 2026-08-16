#!/usr/bin/env lua

local script_path = debug.getinfo(1, "S").source:gsub("^@", "")
local script_dir = script_path:match("^(.*)/[^/]+$")
local util_boot = dofile(script_dir .. "/../../packaging/reqpack/plugin/lib/util.lua")
local repo_root = util_boot.join_path(script_dir, "..", "..")

local util = dofile(repo_root .. "/packaging/reqpack/plugin/lib/util.lua")
local hash = dofile(repo_root .. "/packaging/reqpack/plugin/lib/hash.lua")

local function read_metadata_from_archive(archive_path)
  local ok, output = util.run_shell(
    "tar -xOf " .. util.shell_quote(archive_path) .. " ./metadata.json 2>/dev/null || tar -xOf "
      .. util.shell_quote(archive_path) .. " metadata.json"
  )
  if not ok then
    error("metadata.json missing in " .. archive_path)
  end

  local function capture(pattern)
    return output:match(pattern)
  end

  local metadata = {
    name = capture('"name"%s*:%s*"([^"]+)"'),
    organization = capture('"organization"%s*:%s*"([^"]+)"'),
    plugin = capture('"plugin"%s*:%s*"([^"]+)"'),
    version = capture('"version"%s*:%s*"([^"]+)"'),
    release = tonumber(capture('"release"%s*:%s*(%d+)')) or 1,
    revision = tonumber(capture('"revision"%s*:%s*(%d+)')) or 0,
    architecture = capture('"architecture"%s*:%s*"([^"]+)"'),
    system = { capture('"system"%s*:%s*%[%s*"([^"]+)"') or "linux" },
    summary = capture('"summary"%s*:%s*"([^"]+)"') or "",
    url = capture('"url"%s*:%s*"([^"]+)"') or "",
    tags = {},
  }

  for tag in output:gmatch('"tags"%s*:%s*%b[]') do
    for value in tag:gmatch('"([^"]+)"') do
      if value ~= "tags" then
        metadata.tags[#metadata.tags + 1] = value
      end
    end
    break
  end

  if metadata.name == nil then
    error("failed to parse metadata.json from " .. archive_path)
  end
  return metadata
end

local function usage()
  io.stderr:write("Usage: lua scripts/ci/build_plugin_index.lua --dist-dir dist --output dist/index.json\n")
end

local function parse_args(argv)
  local options = {
    dist_dir = "dist",
    output = "dist/index.json",
  }

  local index = 1
  while index <= #argv do
    local arg = argv[index]
    if arg == "--dist-dir" then
      options.dist_dir = argv[index + 1]
      index = index + 2
    elseif arg == "--output" then
      options.output = argv[index + 1]
      index = index + 2
    elseif arg == "-h" or arg == "--help" then
      usage()
      os.exit(0)
    else
      error("unknown argument: " .. tostring(arg))
    end
  end

  options.dist_dir = util.join_path(repo_root, options.dist_dir)
  options.output = util.join_path(repo_root, options.output)
  return options
end

local function main(argv)
  local options = parse_args(argv)
  local packages = {}

  local ok, output = util.run_shell("find " .. util.shell_quote(options.dist_dir) .. " -maxdepth 1 -name '*.rqp' | sort")
  if not ok then
    error("failed to scan dist directory")
  end

  for line in (output .. "\n"):gmatch("(.-)\n") do
    local archive_path = util.trim(line)
    if archive_path ~= "" then
      local metadata = read_metadata_from_archive(archive_path)
      packages[#packages + 1] = {
        name = metadata.name,
        organization = metadata.organization,
        plugin = metadata.plugin,
        version = metadata.version,
        release = metadata.release,
        revision = metadata.revision,
        architecture = metadata.architecture,
        system = metadata.system,
        summary = metadata.summary,
        url = metadata.url,
        packageSha256 = hash.sha256_file(archive_path),
        tags = metadata.tags or {},
      }
    end
  end

  local index = {
    schemaVersion = 1,
    packages = packages,
  }

  local lines = { "{", '  "schemaVersion": 1,', '  "packages": [' }
  for package_index, package_entry in ipairs(packages) do
    local system_json = string.format('["%s"]', package_entry.system[1])
    local tags = {}
    for _, tag in ipairs(package_entry.tags or {}) do
      tags[#tags + 1] = string.format('"%s"', tag)
    end
    lines[#lines + 1] = "    {"
    lines[#lines + 1] = string.format('      "name": %q,', package_entry.name)
    lines[#lines + 1] = string.format('      "organization": %q,', package_entry.organization)
    lines[#lines + 1] = string.format('      "plugin": %q,', package_entry.plugin)
    lines[#lines + 1] = string.format('      "version": %q,', package_entry.version)
    lines[#lines + 1] = string.format('      "release": %d,', package_entry.release)
    lines[#lines + 1] = string.format('      "revision": %d,', package_entry.revision)
    lines[#lines + 1] = string.format('      "architecture": %q,', package_entry.architecture)
    lines[#lines + 1] = '      "system": ' .. system_json .. ","
    lines[#lines + 1] = string.format('      "summary": %q,', package_entry.summary)
    lines[#lines + 1] = string.format('      "url": %q,', package_entry.url)
    lines[#lines + 1] = string.format('      "packageSha256": %q,', package_entry.packageSha256)
    lines[#lines + 1] = '      "tags": [' .. table.concat(tags, ", ") .. "]"
    if package_index == #packages then
      lines[#lines + 1] = "    }"
    else
      lines[#lines + 1] = "    },"
    end
  end
  lines[#lines + 1] = "  ]"
  lines[#lines + 1] = "}"
  lines[#lines + 1] = ""

  util.mkdir_p(options.output:match("^(.*)/[^/]+$") or ".")
  util.write_file(options.output, table.concat(lines, "\n"))
  print(options.output)
  return 0
end

local ok, err = pcall(main, { ... })
if not ok then
  io.stderr:write(tostring(err) .. "\n")
  os.exit(1)
end
