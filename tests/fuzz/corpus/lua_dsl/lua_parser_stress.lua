-- Parser stress: long strings, nested comments, escaped quotes.
local long = string.rep("x", 4096)
--[[ nested --[[ comment ]] still valid ]]
step({
    name = "s\"q'u\\ote",
    phase = "p\"hase",
    scope = "sc\\ope",
    run = "echo '" .. long .. "'",
    description = [[multi
line
string]],
})
