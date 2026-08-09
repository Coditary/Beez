local keys = {
    "PATH", "HOME", "USER", "SHELL", "PWD", "LANG", "TERM",
    "BEEZ_FZ_0", "BEEZ_FZ_1", "BEEZ_FZ_2", "MISSING_KEY_X",
}
for _, key in ipairs(keys) do
    local value = beez.env(key)
    if value == nil then
        value = "nil"
    end
    task("env-" .. key, "echo " .. key .. "=" .. value)
end
