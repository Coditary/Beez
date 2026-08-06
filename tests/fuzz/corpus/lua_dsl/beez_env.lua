task("noop", "echo " .. (beez.env("PATH") or "missing"))
