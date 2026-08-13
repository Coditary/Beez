local defaults = require("src.defaults")
local command = require("src.command")
local shell = require("src.shell")

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
        by_target[link_entry.target] = link_entry
    end

    local ordered = {}
    for _, target_name in ipairs(defaults.link_order) do
        if link_rank(target_name) <= max_rank then
            local link_entry = by_target[target_name]
            if link_entry ~= nil then
                ordered[#ordered + 1] = link_entry
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
        local code = shell.run(ctx, config.log_prefix_link, command.link(config, link_entry))
        if code ~= 0 then
            return code
        end
    end

    return 0
end

return M
