local util = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/util.lua")
local hash = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/hash.lua")
local archive = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/archive.lua")
local metadata_lib = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/metadata.lua")

local M = {}

local TEMPLATE_FILES = {
  "reqpack.lua",
  "scripts/layout.lua",
  "scripts/install.lua",
  "scripts/remove.lua",
}

local function copy_template_tree(template_root, control_root)
  for _, relative_path in ipairs(TEMPLATE_FILES) do
    local source_path = util.join_path(template_root, relative_path)
    if not util.file_exists(source_path) then
      error("template file not found: " .. source_path)
    end
    local target_path = util.join_path(control_root, relative_path)
    util.mkdir_p(target_path:match("^(.*)/[^/]+$") or control_root)
    local ok, output = util.run_shell(
      "cp -f " .. util.shell_quote(source_path) .. " " .. util.shell_quote(target_path)
    )
    if not ok then
      error("failed to copy template file: " .. relative_path .. " (" .. util.trim(output) .. ")")
    end
  end
end

local function compute_directories(files, payload_root)
  local directories = {}
  local seen = {}
  for _, file_path in ipairs(files) do
    local relative = file_path:sub(#payload_root + 2)
    local current = relative
    while true do
      local parent = current:match("^(.*)/[^/]+$")
      if parent == nil or parent == "" then
        break
      end
      if not seen[parent] then
        seen[parent] = true
        directories[#directories + 1] = parent
      end
      current = parent
    end
  end
  table.sort(directories)
  return directories
end

local function write_payload_manifest(payload_root, script_path)
  local files = util.list_files(payload_root)
  local directories = compute_directories(files, payload_root)
  local lines = { "return {", "  directories = {" }
  for _, directory in ipairs(directories) do
    lines[#lines + 1] = string.format('    "%s",', directory)
  end
  lines[#lines + 1] = "  },"
  lines[#lines + 1] = "  files = {"
  for _, file_path in ipairs(files) do
    local relative = file_path:sub(#payload_root + 2)
    local executable = util.is_executable(file_path) and "true" or "false"
    lines[#lines + 1] = string.format('    { path = "%s", executable = %s },', relative, executable)
  end
  lines[#lines + 1] = "  },"
  lines[#lines + 1] = "}"
  util.write_file(script_path, table.concat(lines, "\n") .. "\n")
end

local function installed_payload_size(payload_root)
  local total = 0
  for _, file_path in ipairs(util.list_files(payload_root)) do
    local handle = io.open(file_path, "rb")
    if handle ~= nil then
      total = total + handle:seek("end")
      handle:close()
    end
  end
  return total
end

function M.package_plugin(options)
  local version = util.normalized_version(options.version)
  local source_dir = options.source_dir
  if not util.file_exists(util.join_path(source_dir, "beez_plugin.lua")) then
    error("source directory must contain beez_plugin.lua: " .. source_dir)
  end

  local template_root = options.template_root
  local output_dir = options.output_dir
  util.mkdir_p(output_dir)

  local archive_name = metadata_lib.archive_name(
    options.organization,
    options.name,
    version,
    options.platform,
    options.arch
  )
  local archive_path = util.join_path(output_dir, archive_name)
  local temp_root = util.join_path(output_dir, ".pack-tmp-" .. options.organization .. "-" .. options.name)
  util.remove_tree(temp_root)
  util.mkdir_p(temp_root)

  local payload_root = util.join_path(temp_root, "payload-tree")
  local control_root = util.join_path(temp_root, "control")
  local control_scripts = util.join_path(control_root, "scripts")
  local control_hashes = util.join_path(control_root, "hashes")
  local control_payload = util.join_path(control_root, "payload")

  util.mkdir_p(payload_root)
  util.mkdir_p(control_scripts)
  util.mkdir_p(control_hashes)
  util.mkdir_p(control_payload)
  copy_template_tree(template_root, control_root)

  local copied, copy_output = util.copy_tree(source_dir, payload_root)
  if not copied then
    util.remove_tree(temp_root)
    error("failed to copy plugin source: " .. util.trim(copy_output))
  end

  write_payload_manifest(payload_root, util.join_path(control_scripts, "payload_files.lua"))

  local payload_tar = util.join_path(control_payload, "payload.tar")
  archive.create_tar(payload_root, payload_tar)
  local payload_zst = util.join_path(control_payload, "payload.tar.zst")
  archive.compress_zstd(payload_tar, payload_zst)
  local zst_handle = io.open(payload_zst, "rb")
  local size_compressed = zst_handle:seek("end")
  zst_handle:close()
  local size_installed = installed_payload_size(payload_root)
  local payload_sha256 = hash.sha256_file(payload_zst)
  util.write_file(
    util.join_path(control_hashes, "payload.sha256"),
    payload_sha256 .. " payload/payload.tar.zst\n"
  )
  util.run_shell("rm -f " .. util.shell_quote(payload_tar))

  local metadata = metadata_lib.build_metadata({
    organization = options.organization,
    name = options.name,
    version = version,
    platform = options.platform,
    arch = options.arch,
    source_url = options.source_url,
    homepage = options.homepage,
    summary = options.summary,
    description = options.description,
    license = options.license,
    tags = options.tags,
    size_compressed = size_compressed,
    size_installed = size_installed,
  })

  metadata_lib.render_template(
    util.join_path(template_root, "metadata.json.in"),
    util.join_path(control_root, "metadata.json"),
    metadata
  )

  if util.file_exists(archive_path) then
    util.remove_tree(archive_path)
  end
  archive.create_rqp_archive(control_root, archive_path)
  util.remove_tree(temp_root)
  return archive_path, metadata
end

return M
