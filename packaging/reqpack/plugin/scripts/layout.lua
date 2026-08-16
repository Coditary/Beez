local M = {}

function M.join_path(base, relative)
  if base == nil or base == "" or base == "." then
    return relative
  end
  if relative == nil or relative == "" then
    return base
  end
  if base == "/" then
    return "/" .. relative
  end
  if base:sub(-1) == "/" then
    return base .. relative
  end
  return base .. "/" .. relative
end

function M.shell_quote(value)
  return "'" .. tostring(value):gsub("'", "'\\''") .. "'"
end

function M.trim(value)
  return (tostring(value):gsub("^%s+", ""):gsub("%s+$", ""))
end

function M.cache_home()
  local cache_home = os.getenv("XDG_CACHE_HOME")
  if cache_home ~= nil and cache_home ~= "" then
    return cache_home
  end
  local home = os.getenv("HOME")
  if home == nil or home == "" then
    error("HOME is not set")
  end
  return M.join_path(home, ".cache")
end

function M.plugin_root(context)
  local organization = context.metadata.organization
  local plugin_name = context.metadata.plugin
  local version = context.metadata.version
  return M.join_path(M.cache_home(), "beez", "plugins", organization, plugin_name, version)
end

function M.paths(context)
  return {
    plugin_root = M.plugin_root(context),
  }
end

function M.load_payload_manifest(context)
  local ok, payload = pcall(dofile, M.join_path(context.paths.controlDir, "scripts/payload_files.lua"))
  if not ok then
    return nil, "failed to load payload manifest: " .. tostring(payload)
  end
  return payload
end

function M.ensure_directories(context, directories)
  for _, dir in ipairs(directories) do
    if not context.fs.exists(dir) then
      local ok, result = pcall(context.fs.mkdir, dir)
      if not ok or result == false then
        return false, "failed to create directory: " .. tostring(dir)
      end
    end
  end
  return true
end

return M
