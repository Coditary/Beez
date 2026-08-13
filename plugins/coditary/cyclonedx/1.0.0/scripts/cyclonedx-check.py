#!/usr/bin/env python3
"""Validate a CycloneDX JSON BOM (structural checks)."""

from __future__ import annotations

import json
import sys
from typing import Any


def validate_bom(bom: dict[str, Any]) -> list[str]:
    errors: list[str] = []

    if bom.get("bomFormat") != "CycloneDX":
        errors.append("bomFormat must be 'CycloneDX'")

    spec_version = bom.get("specVersion")
    if not isinstance(spec_version, str) or not spec_version:
        errors.append("specVersion must be a non-empty string")

    components = bom.get("components")
    if components is not None and not isinstance(components, list):
        errors.append("components must be an array when present")

    metadata = bom.get("metadata")
    if metadata is not None and not isinstance(metadata, dict):
        errors.append("metadata must be an object when present")

    dependencies = bom.get("dependencies")
    if dependencies is not None and not isinstance(dependencies, list):
        errors.append("dependencies must be an array when present")

    if isinstance(components, list):
        for index, component in enumerate(components):
            if not isinstance(component, dict):
                errors.append(f"components[{index}] must be an object")
                continue

            if not component.get("name"):
                errors.append(f"components[{index}] missing name")

    return errors


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: cyclonedx-check.py <cyclonedx.json>", file=sys.stderr)
        return 2

    bom_path = sys.argv[1]

    try:
        with open(bom_path, encoding="utf-8") as handle:
            payload = json.load(handle)
    except OSError as exc:
        print(f"failed to read CycloneDX BOM at {bom_path}: {exc}", file=sys.stderr)
        return 1
    except json.JSONDecodeError as exc:
        print(f"failed to parse CycloneDX JSON at {bom_path}: {exc}", file=sys.stderr)
        return 1

    if not isinstance(payload, dict):
        print("CycloneDX BOM root must be a JSON object", file=sys.stderr)
        return 1

    errors = validate_bom(payload)
    if errors:
        print(f"CycloneDX validation failed for {bom_path}:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    component_count = len(payload.get("components", []))
    print(f"CycloneDX BOM valid ({component_count} components)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
