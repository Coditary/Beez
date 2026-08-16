local defaults = require("src.defaults")
local command = require("src.command")

local M = {}

local function link_rank(target_name)
    for index, name in ipairs(defaults.link_order) do
        if name == target_name then
            return index
        end
    end

    return 9999
end

local function sorted_links(links)
    local sorted = {}
    for _, link_entry in ipairs(links) do
        sorted[#sorted + 1] = link_entry
    end

    table.sort(sorted, function(left, right)
        return link_rank(left.target) < link_rank(right.target)
    end)

    return sorted
end

local function find_target_for_output(links, output_path)
    for _, link_entry in ipairs(links) do
        if link_entry.output == output_path then
            return link_entry.target
        end
    end

    return nil
end

local function link_entry_score(link_entry)
    local preferences = {
        { pattern = "/src/core/", score = 10 },
        { pattern = "/src/logging/", score = 20 },
        { pattern = "/src/cli/", score = 30 },
        { pattern = "/src/plugins/host/", score = 40 },
        { pattern = "/src/plugins/lua/", score = 50 },
        { pattern = "/src/plugins/shell/", score = 60 },
        { pattern = "/src/plugins/", score = 70 },
        { pattern = "/src/app/", score = 80 },
        { pattern = "/tests/unit/", score = 90 },
        { pattern = "/tests/integration/", score = 91 },
        { pattern = "/tests/system/", score = 92 },
        { pattern = "/tests/performance/", score = 93 },
        { pattern = "/fuzz/", score = 94 },
        { pattern = "/src/modules/", score = 200 },
        { pattern = "/modules/", score = 210 },
        { pattern = "/libs/", score = 220 },
        { pattern = "/src/", score = 150 },
    }

    local paths = {
        link_entry.link_txt or "",
        link_entry.work_dir or "",
        link_entry.output or "",
    }

    local best_score = 999
    for _, path in ipairs(paths) do
        for _, preference in ipairs(preferences) do
            if path:find(preference.pattern, 1, true) and preference.score < best_score then
                best_score = preference.score
            end
        end
    end

    return best_score
end

local function best_link_entry(entries)
    local best = entries[1]
    local best_score = link_entry_score(best)

    for index = 2, #entries do
        local candidate = entries[index]
        local score = link_entry_score(candidate)
        if score < best_score then
            best = candidate
            best_score = score
        end
    end

    return best
end

local function filter_links(config, profile, links, root)
    local outputs = profile.link_outputs
    if type(outputs) == "function" then
        outputs = outputs(config.build_tree)
    end

    local max_rank = 0
    for _, rel_path in ipairs(outputs) do
        local target = find_target_for_output(links, root .. "/" .. rel_path)
        if target ~= nil then
            local rank = link_rank(target)
            if rank > max_rank then
                max_rank = rank
            end
        end
    end

    if max_rank == 0 then
        max_rank = 9999
    end

    local by_target = {}
    for _, link_entry in ipairs(links) do
        local entries = by_target[link_entry.target]
        if entries == nil then
            entries = {}
            by_target[link_entry.target] = entries
        end
        entries[#entries + 1] = link_entry
    end

    local ordered = {}
    for _, target_name in ipairs(defaults.link_order) do
        if link_rank(target_name) <= max_rank then
            local entries = by_target[target_name]
            if entries ~= nil then
                ordered[#ordered + 1] = best_link_entry(entries)
            end
        end
    end

    return ordered
end

function M.run(ctx, config, profile, index, root)
    local links = filter_links(config, profile, index.links or {}, root)
    if #links == 0 then
        print(config.log_prefix_link .. " no link targets found in index")
        return 1
    end

    for _, link_entry in ipairs(links) do
        print(config.log_prefix_link .. " linking: " .. link_entry.target)
        local code = beez.shell.run(ctx, config.log_prefix_link, command.link(config, link_entry))
        if code ~= 0 then
            return code
        end
    end

    if profile.post_link ~= nil then
        local post_link_cmd = profile.post_link(config.build_tree)
        if post_link_cmd ~= nil and post_link_cmd ~= "" then
            print(config.log_prefix_link .. " post-link: " .. post_link_cmd)
            local post_code = beez.shell.run(ctx, config.log_prefix_link, post_link_cmd)
            if post_code ~= 0 then
                return post_code
            end
        end
    end

    return 0
end

return M
