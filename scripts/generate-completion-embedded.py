#!/usr/bin/env python3
"""Thin wrapper around Shared-Cpp completion/scripts/generate_embedded_sources.py."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

FILES = (
    "beez.bash",
    "beez.zsh",
    "activate.bash",
    "activate.zsh",
    "install-lib.sh",
)


def shared_generator() -> Path:
    return (
        Path(__file__).resolve().parents[3]
        / "shared"
        / "Shared-Cpp"
        / "completion"
        / "scripts"
        / "generate_embedded_sources.py"
    )


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: generate-completion-embedded.py <source-dir> <output.cpp>", file=sys.stderr)
        return 1

    generator = shared_generator()
    if not generator.is_file():
        print(f"error: shared generator not found: {generator}", file=sys.stderr)
        return 1

    command = [
        sys.executable,
        str(generator),
        sys.argv[1],
        sys.argv[2],
        "--header",
        "beez/cli/completion/completion_embedded.hpp",
        "--namespace",
        "beez::cli",
        "--struct",
        "EmbeddedCompletionFile",
        "--function",
        "embeddedCompletionFiles",
        *FILES,
    ]
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
