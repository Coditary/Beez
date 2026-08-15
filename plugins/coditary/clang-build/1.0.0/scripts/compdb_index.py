#!/usr/bin/env python3
"""Generate compile/link shell scripts and a Lua index from compile_commands.json."""

from __future__ import annotations

import hashlib
import json
import os
import re
import sys
from pathlib import Path


def lua_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def target_from_output(output_path: str) -> str | None:
    marker = "CMakeFiles/"
    if marker not in output_path:
        return None
    fragment = output_path.split(marker, 1)[1]
    if ".dir/" not in fragment:
        return None
    return fragment.split(".dir/", 1)[0]


def work_dir_for_link(link_txt: Path) -> Path:
    return link_txt.parent.parent.parent


def parse_link_output(command: str, work_dir: Path) -> str:
    match = re.search(r"(?:^|\s)-o\s+([^\s]+)", command)
    if match:
        output = Path(match.group(1))
        if not output.is_absolute():
            output = (work_dir / output).resolve()
        return str(output)

    match = re.search(r"\b(?:llvm-)?ar\s+qc\s+([^\s]+)", command)
    if match:
        output = Path(match.group(1))
        if not output.is_absolute():
            output = (work_dir / output).resolve()
        return str(output)

    return ""


def normalize_tool(command: str) -> str:
    command = re.sub(r"/ccache/", "/", command)
    command = re.sub(r"^\S*ccache\s+", "", command)
    return command


def replace_compiler(command: str, cxx: str, cc: str) -> str:
    parts = command.split(None, 1)
    if not parts:
        return command

    first = parts[0]
    rest = parts[1] if len(parts) > 1 else ""

    if "clang++" in first or first.endswith("g++"):
        return cxx + (" " + rest if rest else "")
    if "clang" in first or first.endswith("gcc"):
        return cc + (" " + rest if rest else "")

    return command


def write_script(path: Path, work_dir: Path, command: str) -> None:
    cxx = os.environ.get("CXX", "clang++")
    cc = os.environ.get("CC", "clang")
    command = replace_compiler(command, cxx, cc)
    path.parent.mkdir(parents=True, exist_ok=True)

    preamble: list[str] = []
    archive_match = re.search(r"\b(?:llvm-)?ar\s+qc\s+([^\s]+)", command)
    if archive_match:
        preamble.append(f"rm -f {json.dumps(archive_match.group(1))}")

    path.write_text(
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                f"cd {json.dumps(str(work_dir))}",
                *preamble,
                command,
                "",
            ]
        ),
        encoding="utf-8",
    )


def script_name(prefix: str, key: str) -> str:
    digest = hashlib.sha1(key.encode("utf-8")).hexdigest()[:16]
    return f"{prefix}_{digest}.sh"


def main() -> int:
    if len(sys.argv) != 3:
        print(
            "usage: compdb_index.py <build_tree> <output.lua>",
            file=sys.stderr,
        )
        return 2

    build_tree = Path(sys.argv[1]).resolve()
    output_lua = Path(sys.argv[2]).resolve()
    compdb_path = build_tree / "compile_commands.json"
    scripts_dir = build_tree / ".beez-clang-scripts"

    if not compdb_path.is_file():
        print(f"ERROR: missing {compdb_path}", file=sys.stderr)
        return 1

    if scripts_dir.exists():
        for child in scripts_dir.rglob("*.sh"):
            child.unlink()
    scripts_dir.mkdir(parents=True, exist_ok=True)

    with compdb_path.open(encoding="utf-8") as handle:
        compdb = json.load(handle)

    entries: list[dict[str, str]] = []
    for item in compdb:
        source = item.get("file", "")
        object_rel = item.get("output", "")
        command = item.get("command", "")
        directory = item.get("directory", "")
        if not source or not object_rel or not command or not directory:
            continue

        work_dir = Path(directory).resolve()
        object_path = (work_dir / object_rel).resolve()
        output_for_target = str(object_path)

        target = target_from_output(output_for_target)
        if target is None:
            target = target_from_output(object_rel)
        if target is None:
            continue

        normalized = normalize_tool(command)
        script_path = scripts_dir / script_name("compile", source)
        write_script(script_path, work_dir, normalized)

        entries.append(
            {
                "file": str(Path(source).resolve()),
                "object": str(object_path),
                "target": target,
                "work_dir": str(work_dir),
                "script": str(script_path.resolve()),
            }
        )

    links: list[dict[str, str]] = []
    for link_txt in sorted(build_tree.rglob("CMakeFiles/*.dir/link.txt")):
        target = link_txt.parent.name.replace(".dir", "")
        work_dir = work_dir_for_link(link_txt)
        command = normalize_tool(link_txt.read_text(encoding="utf-8").strip())
        output_path = parse_link_output(command, work_dir)
        script_path = scripts_dir / script_name("link", target + ":" + str(link_txt))
        write_script(script_path, work_dir, command)
        links.append(
            {
                "target": target,
                "link_txt": str(link_txt.resolve()),
                "work_dir": str(work_dir.resolve()),
                "output": output_path,
                "script": str(script_path.resolve()),
            }
        )

    lines = ["return {", "  entries = {"]
    for entry in entries:
        lines.append(
            "    {"
            f'file = "{lua_escape(entry["file"])}", '
            f'object = "{lua_escape(entry["object"])}", '
            f'target = "{lua_escape(entry["target"])}", '
            f'work_dir = "{lua_escape(entry["work_dir"])}", '
            f'script = "{lua_escape(entry["script"])}"'
            " },"
        )
    lines.append("  },")
    lines.append("  links = {")
    for link in links:
        lines.append(
            "    {"
            f'target = "{lua_escape(link["target"])}", '
            f'link_txt = "{lua_escape(link["link_txt"])}", '
            f'work_dir = "{lua_escape(link["work_dir"])}", '
            f'output = "{lua_escape(link["output"])}", '
            f'script = "{lua_escape(link["script"])}"'
            " },"
        )
    lines.append("  },")
    lines.append("}")

    output_lua.parent.mkdir(parents=True, exist_ok=True)
    output_lua.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
