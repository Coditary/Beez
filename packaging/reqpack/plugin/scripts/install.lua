local layout = dofile(context.paths.controlDir .. "/scripts/layout.lua")

local payload, payload_error = layout.load_payload_manifest(context)
if payload == nil then
  context.tx.failed(payload_error)
  return false
end

local paths = layout.paths(context)
local ok, dir_error = layout.ensure_directories(context, { paths.plugin_root })
if not ok then
  context.tx.failed(dir_error)
  return false
end

for _, entry in ipairs(payload.files or {}) do
  local source = layout.join_path(context.paths.payloadDir, entry.path)
  local target = layout.join_path(paths.plugin_root, entry.path)

  if not context.fs.exists(source) then
    context.tx.failed("missing payload file: " .. tostring(entry.path))
    return false
  end

  local parent = target:match("^(.*)/[^/]+$")
  if parent ~= nil and parent ~= "" then
    local mkdir_ok, mkdir_result = pcall(context.fs.mkdir, parent)
    if not mkdir_ok or mkdir_result == false then
      context.tx.failed("failed to create plugin directory: " .. tostring(parent))
      return false
    end
  end

  local copy_ok, copy_result = pcall(context.fs.copy, source, target)
  if not copy_ok or copy_result == false then
    context.tx.failed("failed to copy plugin file: " .. tostring(entry.path))
    return false
  end

  if entry.executable then
    local chmod_result = context.exec.run("chmod +x " .. layout.shell_quote(target))
    if not chmod_result.success then
      local message = chmod_result.stderr ~= "" and chmod_result.stderr or ("failed to mark executable: " .. tostring(entry.path))
      context.tx.failed(message)
      return false
    end
  end
end

local entry_path = layout.join_path(paths.plugin_root, "beez_plugin.lua")
if not context.fs.exists(entry_path) then
  context.tx.failed("installed plugin is missing beez_plugin.lua")
  return false
end

return true
