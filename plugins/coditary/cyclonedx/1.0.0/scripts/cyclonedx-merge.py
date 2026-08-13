#!/usr/bin/env python3
"""Merge multiple CycloneDX JSON BOMs into one."""

from __future__ import annotations

import json
import sys
import uuid
from typing import Any


def component_key(component: dict[str, Any]) -> str:
    bom_ref = component.get("bom-ref")
    if isinstance(bom_ref, str) and bom_ref:
        return bom_ref

    name = component.get("name", "")
    version = component.get("version", "")
    return f"{name}@{version}"


def merge_components(boms: list[dict[str, Any]]) -> list[dict[str, Any]]:
    merged: dict[str, dict[str, Any]] = {}

    for bom in boms:
        components = bom.get("components", [])
        if not isinstance(components, list):
            continue

        for component in components:
            if not isinstance(component, dict):
                continue
            merged[component_key(component)] = component

    return sorted(merged.values(), key=lambda item: (item.get("name", ""), item.get("version", "")))


def merge_dependencies(boms: list[dict[str, Any]]) -> list[dict[str, Any]]:
    merged: dict[str, set[str]] = {}

    for bom in boms:
        dependencies = bom.get("dependencies", [])
        if not isinstance(dependencies, list):
            continue

        for entry in dependencies:
            if not isinstance(entry, dict):
                continue

            ref = entry.get("ref")
            depends_on = entry.get("dependsOn", [])
            if not isinstance(ref, str) or not ref:
                continue
            if not isinstance(depends_on, list):
                continue

            bucket = merged.setdefault(ref, set())
            for dependency in depends_on:
                if isinstance(dependency, str):
                    bucket.add(dependency)

    output: list[dict[str, Any]] = []
    for ref in sorted(merged):
        output.append({"ref": ref, "dependsOn": sorted(merged[ref])})

    return output


def pick_metadata(boms: list[dict[str, Any]]) -> dict[str, Any]:
    for bom in boms:
        metadata = bom.get("metadata")
        if isinstance(metadata, dict):
            return dict(metadata)

    return {}


def merge_boms(boms: list[dict[str, Any]]) -> dict[str, Any]:
    metadata = pick_metadata(boms)
    spec_version = "1.4"

    for bom in boms:
        version = bom.get("specVersion")
        if isinstance(version, str) and version:
            spec_version = version
            break

    merged = {
        "$schema": f"http://cyclonedx.org/schema/bom-{spec_version}.schema.json",
        "bomFormat": "CycloneDX",
        "specVersion": spec_version,
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "version": 1,
        "metadata": metadata,
        "components": merge_components(boms),
        "dependencies": merge_dependencies(boms),
    }

    tools = metadata.get("tools")
    if isinstance(tools, list):
        metadata_tools = list(tools)
    else:
        metadata_tools = []

    metadata_tools.append({
        "vendor": "Coditary",
        "name": "beez-cyclonedx-merge",
        "version": "1",
    })
    merged["metadata"]["tools"] = metadata_tools

    return merged


def load_bom(path: str) -> dict[str, Any]:
    with open(path, encoding="utf-8") as handle:
        payload = json.load(handle)

    if not isinstance(payload, dict):
        raise ValueError(f"{path}: root must be a JSON object")

    return payload


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: cyclonedx-merge.py <output.json> <input1.json> [input2.json ...]",
              file=sys.stderr)
        return 2

    output_path = sys.argv[1]
    input_paths = sys.argv[2:]

    if not input_paths:
        print("cyclonedx-merge.py requires at least one input BOM", file=sys.stderr)
        return 2

    boms: list[dict[str, Any]] = []
    for path in input_paths:
        try:
            boms.append(load_bom(path))
        except OSError as exc:
            print(f"failed to read BOM at {path}: {exc}", file=sys.stderr)
            return 1
        except (json.JSONDecodeError, ValueError) as exc:
            print(f"failed to parse BOM at {path}: {exc}", file=sys.stderr)
            return 1

    merged = merge_boms(boms)

    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(merged, handle, indent=2)
        handle.write("\n")

    print(f"merged {len(input_paths)} BOM(s) → {output_path} ({len(merged['components'])} components)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
