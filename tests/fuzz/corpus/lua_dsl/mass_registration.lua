for i = 1, 128 do
    task("bulk-" .. i, "echo " .. i)
end
for i = 1, 64 do
    workflow("wf-" .. i, {})
end
