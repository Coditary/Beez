local M = {}

local DEFAULT_PARALLELISM = 16

local function entry_key(config, source_path)
    if config.cache_key ~= nil and config.cache_key ~= "" then
        return config.cache_key .. ":" .. source_path
    end

    return source_path
end

local function is_cached(ctx, config, source_path)
    if config.cache_key ~= nil and config.cache_key ~= "" then
        return ctx.success_cached(entry_key(config, source_path))
    end

    return ctx.file_success_cached(source_path)
end

local function cache_success(ctx, config, source_path, duration)
    if config.cache_key ~= nil and config.cache_key ~= "" then
        ctx.cache_success(entry_key(config, source_path))
        return
    end

    ctx.cache_file_success(source_path, duration)
end

local function record_miss(ctx, config, source_path)
    if config.cache_key ~= nil and config.cache_key ~= "" then
        ctx.record_cache_miss(entry_key(config, source_path))
        return
    end

    ctx.record_file_cache_miss(source_path)
end

local function job_failed(config, tool_output, exit_code)
    local output = config.output
    if output ~= nil and output.has_issues ~= nil then
        if output.has_issues(tool_output, exit_code) then
            return true
        end
    elseif exit_code ~= 0 then
        return true
    end

    if config.warnings_as_errors and exit_code ~= 0 then
        return true
    end

    return false
end

local function failure_summary(config, source_path, exit_code, tool_output)
    local output = config.output
    if output ~= nil and output.describe_failure ~= nil then
        return output.describe_failure(source_path, exit_code, tool_output)
    end

    if exit_code ~= nil and exit_code ~= 0 then
        return config.log_prefix .. " failed: " .. source_path .. " (exit " .. tostring(exit_code) .. ")"
    end

    return config.log_prefix .. " issues while checking " .. source_path
end

local function status_log(verbose, message)
    if verbose then
        print(message)
    end
end

local function log_failure(ctx, message)
    if message == nil or message == "" then
        return
    end

    if type(ctx.log_failure) == "function" then
        if message:sub(-1) ~= "\n" then
            message = message .. "\n"
        end
        ctx:log_failure(message)
        return
    end

    print(message)
end

local function failure_output(config, tool_output, exit_code)
    local output = config.output
    if output == nil then
        return tool_output
    end

    if output.filter_issues ~= nil then
        local issue_output = output.filter_issues(tool_output)
        if issue_output ~= "" then
            return issue_output
        end
    end

    if output.filter_failure ~= nil then
        local filtered = output.filter_failure(tool_output)
        if filtered ~= "" then
            return filtered
        end
    end

    if tool_output == "" then
        return ""
    end

    if output.filter_verbose ~= nil then
        return output.filter_verbose(tool_output)
    end

    return tool_output
end

local function print_job_output(config, ctx, verbose, source_path, tool_output, exit_code, failed)
    local output = config.output

    if failed then
        local parts = { failure_summary(config, source_path, exit_code, tool_output) }
        local details = failure_output(config, tool_output, exit_code)
        if details ~= "" then
            parts[#parts + 1] = details
        end

        log_failure(ctx, table.concat(parts, "\n"))
        return
    end

    if verbose and output ~= nil and output.filter_verbose ~= nil then
        local verbose_output = output.filter_verbose(tool_output)
        if verbose_output ~= "" then
            print(verbose_output)
        end
    end
end

local function action_label(config)
    if config.action ~= nil and config.action ~= "" then
        return config.action
    end

    return "checking"
end

local function process_batch(ctx, config, verbose, batch_paths, failed_paths)
    local jobs = {}
    local failed = 0
    local label = action_label(config)

    for _, source_path in ipairs(batch_paths) do
        status_log(verbose, config.log_prefix .. " " .. label .. ": " .. source_path)
        jobs[#jobs + 1] = {
            path = source_path,
            handle = ctx:spawn({
                cmd = config.build_cmd(source_path),
            }),
        }
    end

    if #jobs == 0 then
        return 0
    end

    local handles = {}
    for index, job in ipairs(jobs) do
        handles[index] = job.handle
    end

    local results = ctx:wait_all(handles, { exitCode = true, output = true, duration = true })

    for index, result in ipairs(results) do
        local source_path = jobs[index].path
        local tool_output = result.output or ""
        local exit_code = result.exitCode or 0
        local failed_job = job_failed(config, tool_output, exit_code)

        print_job_output(config, ctx, verbose, source_path, tool_output, exit_code, failed_job)

        if failed_job then
            record_miss(ctx, config, source_path)
            failed_paths[#failed_paths + 1] = source_path
            failed = failed + 1
        else
            cache_success(ctx, config, source_path, result.duration)
        end
    end

    return failed
end

function M.run(ctx, config)
    if type(config.build_cmd) ~= "function" then
        error("beez.increment.run config.build_cmd must be a function")
    end

    if config.prereq ~= nil then
        local prereq_code = config.prereq()
        if prereq_code ~= nil then
            return prereq_code
        end
    end

    local files = ctx.glob(config.patterns)
    if #files == 0 then
        status_log(ctx.verbose == true, config.log_prefix .. " no files matched")
        return 0
    end

    local misses = ctx.get_cache_misses()
    if #misses > 0 then
        status_log(ctx.verbose == true, config.log_prefix .. " re-checking from previous failures:")
        for _, entry in ipairs(misses) do
            status_log(ctx.verbose == true, "  - " .. entry)
        end
    end

    local verbose = ctx.verbose == true
    local checked = 0
    local skipped = 0
    local failed = 0
    local failed_paths = {}
    local batch_paths = {}
    local parallelism = config.parallelism
    if config.spawn_all then
        parallelism = nil
    elseif parallelism == nil then
        parallelism = DEFAULT_PARALLELISM
    end

    for _, source_path in ipairs(files) do
        if is_cached(ctx, config, source_path) then
            status_log(verbose, config.log_prefix .. " skip (cached): " .. source_path)
            skipped = skipped + 1
        else
            checked = checked + 1
            batch_paths[#batch_paths + 1] = source_path

            if parallelism ~= nil and parallelism > 0 and #batch_paths >= parallelism then
                failed = failed + process_batch(ctx, config, verbose, batch_paths, failed_paths)
                batch_paths = {}
            end
        end
    end

    if #batch_paths > 0 then
        failed = failed + process_batch(ctx, config, verbose, batch_paths, failed_paths)
    end

    local summary = config.log_prefix .. " summary: checked=" .. checked .. " skipped=" .. skipped ..
        " failed=" .. failed
    if failed > 0 then
        local parts = { summary, config.log_prefix .. " failures:" }
        for _, path in ipairs(failed_paths) do
            parts[#parts + 1] = "  - " .. path
        end
        log_failure(ctx, table.concat(parts, "\n"))
    elseif verbose then
        print(summary)
    end

    if failed > 0 then
        return 1
    end
    return 0
end

return M
