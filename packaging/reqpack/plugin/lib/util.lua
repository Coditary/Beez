local M = {}

function M.trim(value)
  return (tostring(value or ""):gsub("^%s+", ""):gsub("%s+$", ""))
end

function M.starts_with(value, prefix)
  local text = tostring(value or "")
  local expected = tostring(prefix or "")
  return text:sub(1, #expected) == expected
end

function M.join_path(...)
  local parts = {}
  for index = 1, select("#", ...) do
    local part = M.trim(select(index, ...))
    if part ~= "" then
      if #parts == 0 then
        parts[#parts + 1] = part:gsub("[/\\]+$", "")
      else
        parts[#parts + 1] = part:gsub("^[/\\]+", ""):gsub("[/\\]+$", "")
      end
    end
  end
  if #parts == 0 then
    return "."
  end
  return table.concat(parts, "/")
end

function M.shell_quote(value)
  return "'" .. tostring(value or ""):gsub("'", "'\\''") .. "'"
end

function M.normalized_version(raw)
  local text = M.trim(raw)
  if M.starts_with(text, "v") then
    return text:sub(2)
  end
  return text
end

function M.run_shell(command)
  local handle = io.popen(command .. " 2>&1")
  if handle == nil then
    return false, "", 1
  end
  local output = handle:read("*a") or ""
  local ok, _, code = handle:close()
  if ok == true then
    return true, output, 0
  end
  return false, output, tonumber(code) or 1
end

function M.shell_success(command)
  local success = M.run_shell(command)
  return success
end

function M.mkdir_p(path)
  return M.run_shell("mkdir -p " .. M.shell_quote(path))
end

function M.read_file(path)
  local file = io.open(path, "rb")
  if file == nil then
    return nil
  end
  local content = file:read("*a") or ""
  file:close()
  return content
end

function M.write_file(path, content)
  local parent = path:match("^(.*)/[^/]+$")
  if parent ~= nil and parent ~= "" then
    local ok = M.mkdir_p(parent)
    if not ok then
      return false
    end
  end
  local file, err = io.open(path, "wb")
  if file == nil then
    return false, err
  end
  file:write(content or "")
  file:close()
  return true
end

function M.file_exists(path)
  local file = io.open(path, "rb")
  if file == nil then
    return false
  end
  file:close()
  return true
end

function M.is_executable(path)
  local ok, output = M.run_shell("test -x " .. M.shell_quote(path) .. " && echo yes")
  return ok and M.trim(output) == "yes"
end

function M.list_files(root)
  local files = {}
  local ok, output = M.run_shell("find " .. M.shell_quote(root) .. " -type f | sort")
  if not ok then
    return files
  end
  for line in (output .. "\n"):gmatch("(.-)\n") do
    local trimmed = M.trim(line)
    if trimmed ~= "" then
      files[#files + 1] = trimmed
    end
  end
  return files
end

function M.copy_tree(source, destination)
  local ok = M.mkdir_p(destination)
  if not ok then
    return false
  end
  return M.run_shell("cp -R " .. M.shell_quote(M.join_path(source, ".")) .. " " .. M.shell_quote(destination))
end

function M.remove_tree(path)
  return M.run_shell("rm -rf " .. M.shell_quote(path))
end

function M.repo_root_from_script(level)
  local source = debug.getinfo(level or 2, "S").source
  if M.starts_with(source, "@") then
    source = source:sub(2)
  end
  local script_dir = source:match("^(.*)/[^/]+$") or "."
  return M.join_path(script_dir, "..", "..", "..", "..")
end

return M
