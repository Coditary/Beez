local util = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/util.lua")

local M = {}

function M.sha256_file(path)
  local ok, output = util.run_shell("sha256sum " .. util.shell_quote(path))
  if ok then
    local digest = output:match("^(%x+)")
    if digest ~= nil then
      return digest
    end
  end

  ok, output = util.run_shell("openssl dgst -sha256 " .. util.shell_quote(path))
  if ok then
    local digest = output:match("= (%x+)")
    if digest ~= nil then
      return digest
    end
  end

  error("failed to compute sha256 for " .. tostring(path))
end

return M
