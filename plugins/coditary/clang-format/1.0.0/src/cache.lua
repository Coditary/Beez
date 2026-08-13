local command = require("src.command")

local M = {}

function M.run(ctx, config, mode)
    local files = ctx.glob(config.patterns)
    if #files == 0 then
        print(config.log_prefix .. " no files matched")
        return 0
    end

    local misses = ctx.get_cache_misses()
    if #misses > 0 then
        print(config.log_prefix .. " re-checking from previous failures:")
        for _, entry in ipairs(misses) do
            print("  - " .. entry)
        end
    end

    local checked = 0
    local skipped = 0
    local failed = 0
    local pending_jobs = {}
    local pending_paths = {}

    for _, source_path in ipairs(files) do
        if ctx.file_success_cached(source_path) then
            print(config.log_prefix .. " skip (cached): " .. source_path)
            skipped = skipped + 1
        else
            print(config.log_prefix .. (mode == "check" and " checking: " or " applying: ") ..
                source_path)

            checked = checked + 1
            pending_jobs[#pending_jobs + 1] = ctx:spawn({
                cmd = command.build(config, source_path, mode),
            })
            pending_paths[#pending_paths + 1] = source_path
        end
    end

    if #pending_jobs > 0 then
        local results = ctx:wait_all(pending_jobs, { exitCode = true, duration = true })
        for job_index, result in ipairs(results) do
            local source_path = pending_paths[job_index]

            if result.exitCode ~= 0 then
                ctx.record_file_cache_miss(source_path)
                failed = failed + 1
            else
                ctx.cache_file_success(source_path, result.duration)
            end
        end
    end

    print(config.log_prefix .. " summary: checked=" .. checked .. " skipped=" .. skipped ..
        " failed=" .. failed)

    if failed > 0 then
        return 1
    end
    return 0
end

return M
