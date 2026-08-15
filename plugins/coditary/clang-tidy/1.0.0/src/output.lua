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

    if line:find("warnings generated", 1, true) then
        return true
    end

    if line:find("Suppressed ", 1, true) then
        return true
    end

    if line:find("Use -header-filter", 1, true) then
        return true
    end

    if line:find("Use -system-headers", 1, true) then
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

function M.matches_project_path(line)
    if line:find("/src/", 1, true) or line:find("/include/", 1, true) or
        line:find("/tests/", 1, true) then
        return true
    end

    return line:find("^src/", 1, true) ~= nil or line:find("^include/", 1, true) ~= nil or
        line:find("^tests/", 1, true) ~= nil
end

function M.is_user_diagnostic(config, line)
    if not M.matches_project_path(line) then
        return false
    end

    return line:find("warning:", 1, true) ~= nil or line:find("error:", 1, true) ~= nil
end

function M.is_diagnostic_context(line)
    if line:match("^%s*%d+%s*|") then
        return true
    end

    if line:match("^%s*|") then
        return true
    end

    return false
end

function M.is_diagnostic_header(config, line)
    if M.line_excluded(config, line) then
        return false
    end

    return M.is_user_diagnostic(config, line)
end

function M.is_processing_error(line)
    return line:find("Error while processing", 1, true) ~= nil
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

    local blocks = {}
    local current = nil
    local plain = M.strip_ansi(text)

    local function flush_block()
        if current ~= nil then
            blocks[#blocks + 1] = current
            current = nil
        end
    end

    for line in plain:gmatch("[^\r\n]+") do
        if M.is_noise_line(line) then
            flush_block()
            goto continue
        end

        if M.is_diagnostic_header(config, line) then
            flush_block()
            current = { M.relativize_diagnostic(config.project_root, line) }
        elseif current ~= nil and M.is_diagnostic_context(line) then
            current[#current + 1] = line
        elseif M.is_processing_error(line) then
            flush_block()
            current = { M.relativize_diagnostic(config.project_root, line) }
        else
            flush_block()
        end

        ::continue::
    end

    flush_block()
    return blocks
end

function M.describe_failure(config, source_path, exit_code, text)
    local issues = M.collect_issues(config, text)
    local count = #issues
    local prefix = config.log_prefix or "[clang-tidy]"

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

            if line:find("error:", 1, true) or line:find("Error while processing", 1, true) then
                return true
            end

            ::continue_exit::
        end

        return true
    end

    return false
end

function M.filter_issues(config, text)
    local blocks = M.collect_issues(config, text)
    if #blocks == 0 then
        return ""
    end

    local formatted = {}
    for block_index, block in ipairs(blocks) do
        local block_lines = {}
        for line_index, line in ipairs(block) do
            block_lines[line_index] = "  " .. line
        end
        formatted[block_index] = table.concat(block_lines, "\n")
    end

    return table.concat(formatted, "\n")
end

function M.filter_failure(config, text)
    if text == nil or text == "" then
        return ""
    end

    local lines = {}
    local plain = M.strip_ansi(text)

    for line in plain:gmatch("[^\r\n]+") do
        if M.is_noise_line(line) then
            goto continue
        end

        lines[#lines + 1] = line

        ::continue::
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
