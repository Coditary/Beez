#!/usr/bin/env python3
"""Write a shields.io endpoint JSON file from a gcovr --json-summary file."""

from __future__ import annotations

import json
import sys


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: coverage-badge-json.py <summary.json> <badge.json> [min_line_coverage]",
              file=sys.stderr)
        return 2

    summary_path, badge_path = sys.argv[1], sys.argv[2]
    min_line_coverage = float(sys.argv[3]) if len(sys.argv) > 3 else 85.0

    with open(summary_path, encoding="utf-8") as handle:
        summary = json.load(handle)

    line_percent = float(summary["line_percent"])
    badge = {
        "schemaVersion": 1,
        "label": "coverage",
        "message": f"{line_percent:.1f}%",
        "color": "brightgreen" if line_percent >= min_line_coverage else "red",
    }

    with open(badge_path, "w", encoding="utf-8") as handle:
        json.dump(badge, handle)
        handle.write("\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
