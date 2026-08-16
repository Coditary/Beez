local layout = dofile(context.paths.controlDir .. "/scripts/layout.lua")

local paths = layout.paths(context)
local remove_result = context.exec.run("rm -rf " .. layout.shell_quote(paths.plugin_root))
if not remove_result.success then
  local message = remove_result.stderr ~= "" and remove_result.stderr or "failed to remove installed plugin"
  context.tx.failed(message)
  return false
end

return true
