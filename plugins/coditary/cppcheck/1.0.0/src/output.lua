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

function M.matches_project_path(line)
    if line:find("/src/", 1, true) or line:find("/include/", 1, true) or
        line:find("/tests/", 1, true) then
        return true
    end

    return line:find("^src/", 1, true) ~= nil or line:find("^include/", 1, true) ~= nil or
        line:find("^tests/", 1, true) ~= nil
end

function M.relativize_diagnostic(project_root, line)
    if project_root == nil or project_root == "" then
        return line
    end

    local root = project_root
    if root:sub(-1) ~= "/" then
        root = root .. "/"
    end

    if line:sub(1, #root) == root then
        return line:sub(#root + 1)
    end

    return line
end

function M.collect_issues(config, text)
    if text == nil or text == "" then
        return {}
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
            lines[#lines + 1] = M.relativize_diagnostic(config.project_root, line)
        end

        ::continue::
    end

    return lines
end

function M.describe_failure(config, source_path, exit_code, text)
    local issues = M.collect_issues(config, text)
    local count = #issues
    local prefix = config.log_prefix or "[cppcheck]"

    if exit_code ~= nil and exit_code ~= 0 then
        return string.format(
            "%s failed while checking %s (exit %d, %d issue%s)",
            prefix,
            source_path,
            exit_code,
            count,
            count == 1 and "" or "s"
        )
    end

    if count == 0 then
        return prefix .. " issues while checking " .. source_path
    end

    return string.format(
        "%s found %d issue%s while checking %s",
        prefix,
        count,
        count == 1 and "" or "s",
        source_path
    )
end

function M.is_user_diagnostic(config, line)
    return M.matches_project_path(line) and M.is_diagnostic_line(line)
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
    local issues = M.collect_issues(config, text)
    if #issues == 0 then
        return ""
    end

    local formatted = {}
    for index, line in ipairs(issues) do
        formatted[index] = "  " .. line
    end

    return table.concat(formatted, "\n")
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
