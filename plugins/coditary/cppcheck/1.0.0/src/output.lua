local M = {}

function M.strip_ansi(text)
    if text == nil then
        return ""
    end

    return text:gsub("%\27%[[0-9;]*m", "")
end

function M.is_noise_line(line)
    if line == "" then
        return true
    end

    if line:find("Checking ", 1, true) then
        return true
    end

    return false
end

function M.line_excluded(config, line)
    for _, substring in ipairs(config.exclude_substrings) do
        if line:find(substring, 1, true) then
            return true
        end
    end

    return false
end

function M.is_diagnostic_line(line)
    return line:find("%(error%)", 1, true) or line:find("%(warning%)", 1, true) or
        line:find("%(style%)", 1, true) or line:find("%(performance%)", 1, true) or
        line:find("%(portability%)", 1, true) or line:find("%(information%)", 1, true)
end

function M.is_user_diagnostic(config, line)
    return line:find(config.issue_path_pattern, 1, true) and M.is_diagnostic_line(line)
end

function M.has_issues(config, text, exit_code)
    if text == nil or text == "" then
        return exit_code ~= nil and exit_code ~= 0
    end

    local plain = M.strip_ansi(text)
    for line in plain:gmatch("[^\r\n]+") do
        if M.is_noise_line(line) then
            goto continue
        end

        if M.is_user_diagnostic(config, line) and not M.line_excluded(config, line) then
            return true
        end

        ::continue::
    end

    if exit_code ~= nil and exit_code ~= 0 then
        for line in plain:gmatch("[^\r\n]+") do
            if M.is_noise_line(line) then
                goto continue_exit
            end

            if M.is_diagnostic_line(line) then
                return true
            end

            ::continue_exit::
        end

        return true
    end

    return false
end

function M.filter_issues(config, text)
    if text == nil or text == "" then
        return ""
    end

    local lines = {}
    local plain = M.strip_ansi(text)

    for line in plain:gmatch("[^\r\n]+") do
        if M.is_noise_line(line) then
            goto continue
        end

        if M.line_excluded(config, line) then
            goto continue
        end

        if M.is_user_diagnostic(config, line) then
            lines[#lines + 1] = line
        end

        ::continue::
    end

    if #lines == 0 then
        return ""
    end

    return table.concat(lines, "\n")
end

function M.filter_failure(text)
    if text == nil or text == "" then
        return ""
    end

    local lines = {}
    local plain = M.strip_ansi(text)

    for line in plain:gmatch("[^\r\n]+") do
        if not M.is_noise_line(line) then
            lines[#lines + 1] = line
        end
    end

    if #lines == 0 then
        return ""
    end

    return table.concat(lines, "\n")
end

function M.filter_verbose(text)
    if text == nil or text == "" then
        return ""
    end

    local lines = {}
    local plain = M.strip_ansi(text)

    for line in plain:gmatch("[^\r\n]+") do
        if not M.is_noise_line(line) then
            lines[#lines + 1] = line
        end
    end

    if #lines == 0 then
        return ""
    end

    return table.concat(lines, "\n")
end

return M
