local M = {}

function M.quote(value)
    return "'" .. value:gsub("'", "'\\''") .. "'"
end

function M.run(ctx, log_prefix, cmd)
    local handle = ctx:spawn({ cmd = cmd })
    local result = ctx:wait(handle, { exitCode = true, output = true })

    local exit_code = result.exitCode or 0
    local output = result.output or ""

    if exit_code ~= 0 then
        print(log_prefix .. " failed (exit " .. tostring(exit_code) .. ")")
        if output ~= "" then
            print(output)
        end
        return exit_code, output
    end

    if ctx.verbose == true and output ~= "" then
        print(output)
    end

    return 0, output
end

return M
