local command = require("src.command")

local M = {}

local DEFAULT_PARALLELISM = 16

local function entry_key(config, source_path)
    return config.cache_key .. ":" .. source_path
end

local function is_cached(ctx, config, source_path)
    return ctx.success_cached(entry_key(config, source_path))
end

local function cache_success(ctx, config, source_path, duration)
    ctx.cache_success(entry_key(config, source_path))
end

local function record_miss(ctx, config, source_path)
    ctx.record_cache_miss(entry_key(config, source_path))
end

local function process_batch(ctx, config, batch_entries, failed_paths)
    local jobs = {}
    local failed = 0

    for _, entry in ipairs(batch_entries) do
        print(config.log_prefix_compile .. " compiling: " .. entry.file)
        jobs[#jobs + 1] = {
            file = entry.file,
            handle = ctx:spawn({
                cmd = command.compile(config, entry),
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
        local source_path = jobs[index].file
        local tool_output = result.output or ""
        local exit_code = result.exitCode or 0

        if exit_code ~= 0 then
            print(config.log_prefix_compile .. " failed: " .. source_path .. " (exit " .. tostring(exit_code) .. ")")
            if tool_output ~= "" then
                print(tool_output)
            end
            record_miss(ctx, config, source_path)
            failed_paths[#failed_paths + 1] = source_path
            failed = failed + 1
        else
            cache_success(ctx, config, source_path, result.duration)
        end
    end

    return failed
end

function M.run(ctx, config, index)
    local entries = index.entries
    if #entries == 0 then
        print(config.log_prefix_compile .. " no compile entries in index")
        return 0
    end

    local misses = ctx.get_cache_misses()
    if #misses > 0 then
        print(config.log_prefix_compile .. " re-compiling from previous failures:")
        for _, entry in ipairs(misses) do
            print("  - " .. entry)
        end
    end

    local checked = 0
    local skipped = 0
    local failed = 0
    local failed_paths = {}
    local batch_entries = {}
    local parallelism = config.parallelism or DEFAULT_PARALLELISM

    for _, entry in ipairs(entries) do
        if is_cached(ctx, config, entry.file) then
            print(config.log_prefix_compile .. " skip (cached): " .. entry.file)
            skipped = skipped + 1
        else
            checked = checked + 1
            batch_entries[#batch_entries + 1] = entry

            if #batch_entries >= parallelism then
                failed = failed + process_batch(ctx, config, batch_entries, failed_paths)
                batch_entries = {}
            end
        end
    end

    if #batch_entries > 0 then
        failed = failed + process_batch(ctx, config, batch_entries, failed_paths)
    end

    print(config.log_prefix_compile .. " summary: checked=" .. checked ..
        " skipped=" .. skipped .. " failed=" .. failed)

    if failed > 0 then
        return 1
    end

    return 0
end

return M
