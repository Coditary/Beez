local util = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/util.lua")

local M = {}

function M.create_tar(source_root, tar_path)
  local parent = tar_path:match("^(.*)/[^/]+$")
  if parent ~= nil then
    util.mkdir_p(parent)
  end
  local ok, output, code = util.run_shell(
    "tar -cf " .. util.shell_quote(tar_path) .. " -C " .. util.shell_quote(source_root) .. " ."
  )
  if not ok then
    error("failed to create tar archive (exit " .. tostring(code) .. "): " .. util.trim(output))
  end
end

function M.compress_zstd(tar_path, zst_path)
  local ok, output, code = util.run_shell(
    "zstd -q -f " .. util.shell_quote(tar_path) .. " -o " .. util.shell_quote(zst_path)
  )
  if not ok then
    error("failed to compress payload (exit " .. tostring(code) .. "): " .. util.trim(output))
  end
end

function M.create_rqp_archive(control_root, archive_path)
  local parent = archive_path:match("^(.*)/[^/]+$")
  if parent ~= nil then
    util.mkdir_p(parent)
  end
  local ok, output, code = util.run_shell(
    "tar -cf " .. util.shell_quote(archive_path) .. " -C " .. util.shell_quote(control_root) .. " ."
  )
  if not ok then
    error("failed to create rqp archive (exit " .. tostring(code) .. "): " .. util.trim(output))
  end
end

return M
