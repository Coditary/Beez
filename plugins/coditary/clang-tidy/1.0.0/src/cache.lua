local command = require("src.command")
local output = require("src.output")

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
    if output.has_issues(config, tool_output, exit_code) then
        return true
    end

    if config.warnings_as_errors and exit_code ~= 0 then
        return true
    end

    return false
end

local function print_job_output(config, verbose, source_path, tool_output, exit_code, failed)
    if failed then
        print(config.log_prefix .. " failed: " .. source_path .. " (exit " .. tostring(exit_code) .. ")")

        local issue_output = output.filter_issues(config, tool_output)
        if issue_output ~= "" then
            print(issue_output)
            return
        end

        local failure_output = output.filter_failure(config, tool_output)
        if failure_output ~= "" then
            print(failure_output)
            return
        end

        if tool_output ~= "" then
            print(output.filter_verbose(tool_output))
        end
        return
    end

    if verbose then
        local verbose_output = output.filter_verbose(tool_output)
        if verbose_output ~= "" then
            print(verbose_output)
        end
    end
end

local function process_batch(ctx, config, verbose, batch_paths, failed_paths)
    local jobs = {}
    local failed = 0

    for _, source_path in ipairs(batch_paths) do
        print(config.log_prefix .. " checking: " .. source_path)
        jobs[#jobs + 1] = {
            path = source_path,
            handle = ctx:spawn({
                cmd = command.build(config, source_path),
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

        print_job_output(config, verbose, source_path, tool_output, exit_code, failed_job)

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
    local files = ctx.glob(config.patterns)
    if #files == 0 then
        print(config.log_prefix .. " no files matched")
        return 0
    end

    local compdb_file = config.compdb .. "/compile_commands.json"
    if not beez.fs.exists(compdb_file) then
        print(config.log_prefix .. " compile_commands.json not found under: " .. config.compdb)
        print(config.log_prefix .. " Run: make setup (or make debug) first.")
        return 2
    end

    local misses = ctx.get_cache_misses()
    if #misses > 0 then
        print(config.log_prefix .. " re-checking from previous failures:")
        for _, entry in ipairs(misses) do
            print("  - " .. entry)
        end
    end

    local verbose = ctx.verbose == true
    local checked = 0
    local skipped = 0
    local failed = 0
    local failed_paths = {}
    local batch_paths = {}
    local parallelism = config.parallelism or DEFAULT_PARALLELISM

    for _, source_path in ipairs(files) do
        if is_cached(ctx, config, source_path) then
            print(config.log_prefix .. " skip (cached): " .. source_path)
            skipped = skipped + 1
        else
            checked = checked + 1
            batch_paths[#batch_paths + 1] = source_path

            if #batch_paths >= parallelism then
                failed = failed + process_batch(ctx, config, verbose, batch_paths, failed_paths)
                batch_paths = {}
            end
        end
    end

    if #batch_paths > 0 then
        failed = failed + process_batch(ctx, config, verbose, batch_paths, failed_paths)
    end

    print(config.log_prefix .. " summary: checked=" .. checked .. " skipped=" .. skipped ..
        " failed=" .. failed)

    if #failed_paths > 0 then
        print(config.log_prefix .. " failures:")
        for _, path in ipairs(failed_paths) do
            print("  - " .. path)
        end
    end

    if failed > 0 then
        return 1
    end
    return 0
end

return M
