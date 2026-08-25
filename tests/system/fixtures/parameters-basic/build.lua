parameters("meta.json")
local extra = beez.var.extra and beez.var.extra.flag or "nil"
task("show", "echo greeting=" .. beez.var.greeting .. " host=" .. beez.var.server.host ..
    " port=" .. tostring(beez.var.server.port) .. " flag=" .. extra .. " > result.txt")
