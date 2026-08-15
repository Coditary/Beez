local M = {}

M.python_binary = "python3"
M.cyclonedx_cli = "cyclonedx-cli"

M.sbom_dir = "report/sbom"
M.cyclonedx_json = "report/sbom/cyclonedx.json"
M.merged_json = "report/sbom/cyclonedx-merged.json"

M.check_script = "plugins/coditary/cyclonedx/1.0.0/scripts/cyclonedx-check.py"
M.merge_script = "plugins/coditary/cyclonedx/1.0.0/scripts/cyclonedx-merge.py"

M.log_prefix_check = "[cyclonedx-check]"
M.log_prefix_merge = "[cyclonedx-merge]"

M.check_rev = "1"
M.merge_rev = "1"

M.default_merge_inputs = {
    "report/sbom/cyclonedx.json",
}

return M
