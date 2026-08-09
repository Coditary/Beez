#!/usr/bin/env bash
set -euo pipefail

SUMMARY_JSON="${1:?usage: coverage-github-summary.sh <coverage-summary.json> [min_line_coverage]}"
MIN_LINE_COVERAGE="${2:-${MIN_LINE_COVERAGE:-85}}"
GITHUB_STEP_SUMMARY="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

if [ ! -f "${SUMMARY_JSON}" ]; then
    {
        echo "## Code coverage"
        echo ""
        echo "Coverage report was not generated."
    } >> "${GITHUB_STEP_SUMMARY}"
    exit 0
fi

export SUMMARY_JSON MIN_LINE_COVERAGE GITHUB_STEP_SUMMARY GITHUB_OUTPUT="${GITHUB_OUTPUT:-}"

python3 <<'PY'
import json
import os
import sys
import urllib.parse

summary_path = os.environ["SUMMARY_JSON"]
min_line_coverage = float(os.environ["MIN_LINE_COVERAGE"])

with open(summary_path, encoding="utf-8") as handle:
    summary = json.load(handle)

line_percent = float(summary["line_percent"])
line_covered = int(summary["line_covered"])
line_total = int(summary["line_total"])
meets = line_percent >= min_line_coverage
badge_color = "brightgreen" if meets else "red"
encoded = urllib.parse.quote(f"{line_percent:.1f}%", safe="")
shield_url = f"https://img.shields.io/badge/coverage-{encoded}-{badge_color}"

summary_file = os.environ["GITHUB_STEP_SUMMARY"]
with open(summary_file, "a", encoding="utf-8") as handle:
    handle.write("## Code coverage\n\n")
    handle.write(f"![Coverage badge]({shield_url})\n\n")
    handle.write("| Metric | Value |\n")
    handle.write("|--------|-------|\n")
    handle.write(f"| Line coverage (`src/`) | **{line_percent:.1f}%** |\n")
    handle.write(f"| Lines covered | {line_covered} / {line_total} |\n")
    handle.write(f"| Minimum required | {min_line_coverage:g}% |\n")

output_file = os.environ.get("GITHUB_OUTPUT")
if output_file:
    with open(output_file, "a", encoding="utf-8") as handle:
        handle.write(f"line_percent={line_percent:.1f}\n")
        handle.write(f"badge_color={badge_color}\n")
        handle.write(f"meets_threshold={'true' if meets else 'false'}\n")
        handle.write(f"shield_url={shield_url}\n")
PY
