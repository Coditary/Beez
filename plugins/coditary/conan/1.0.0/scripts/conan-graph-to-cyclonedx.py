#!/usr/bin/env python3
"""Convert `conan graph info --format=json` output to CycloneDX 1.4 JSON."""

from __future__ import annotations

import json
import sys
import uuid
from typing import Any


def conan_purl(name: str, version: str) -> str:
    return f"pkg:conan/{name}@{version}"


def component_ref(name: str, version: str) -> str:
    return conan_purl(name, version)


def should_include(node: dict[str, Any]) -> bool:
    if node.get("recipe") == "Consumer":
        return False
    if node.get("context") != "host":
        return False
    if not node.get("name") or not node.get("version"):
        return False
    return True


def license_entries(node: dict[str, Any]) -> list[dict[str, Any]]:
    license_id = node.get("license")
    if not license_id:
        return []
    return [{"license": {"id": license_id}}]


def build_component(node: dict[str, Any]) -> dict[str, Any]:
    name = node["name"]
    version = node["version"]
    component: dict[str, Any] = {
        "type": "library",
        "bom-ref": component_ref(name, version),
        "name": name,
        "version": version,
        "purl": conan_purl(name, version),
    }

    licenses = license_entries(node)
    if licenses:
        component["licenses"] = licenses

    for field in ("description", "homepage", "url"):
        value = node.get(field)
        if value:
            component[field if field != "url" else "externalReferences"] = (
                [{"type": "website", "url": value}] if field == "url" else value
            )

    return component


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: conan-graph-to-cyclonedx.py <conan-graph.json> <cyclonedx.json>",
              file=sys.stderr)
        return 2

    graph_path, output_path = sys.argv[1], sys.argv[2]

    try:
        with open(graph_path, encoding="utf-8") as handle:
            payload = json.load(handle)
    except OSError as exc:
        print(f"failed to read Conan graph at {graph_path}: {exc}", file=sys.stderr)
        return 1
    except json.JSONDecodeError as exc:
        print(f"failed to parse Conan graph JSON at {graph_path}: {exc}", file=sys.stderr)
        return 1

    graph = payload.get("graph")
    if not isinstance(graph, dict):
        print("invalid Conan graph: missing or invalid graph object", file=sys.stderr)
        return 1

    nodes_raw = graph.get("nodes")
    if not isinstance(nodes_raw, dict):
        print("invalid Conan graph: missing or invalid graph.nodes", file=sys.stderr)
        return 1

    nodes: dict[str, dict[str, Any]] = nodes_raw
    consumer = nodes.get("0")
    if consumer is None:
        print("missing consumer node in Conan graph", file=sys.stderr)
        return 1

    included_nodes = {
        node_id: node for node_id, node in nodes.items() if should_include(node)
    }
    refs_by_id = {
        node_id: component_ref(node["name"], node["version"])
        for node_id, node in included_nodes.items()
    }

    components = [build_component(node) for node in included_nodes.values()]
    components.sort(key=lambda item: (item["name"], item["version"]))

    dependencies: list[dict[str, Any]] = []
    root_ref = component_ref(consumer["name"], consumer["version"])
    direct_refs: list[str] = []

    for dep_id, dep in consumer.get("dependencies", {}).items():
        if dep_id in refs_by_id:
            direct_refs.append(refs_by_id[dep_id])

    if direct_refs:
        dependencies.append({"ref": root_ref, "dependsOn": sorted(set(direct_refs))})

    for node_id, node in included_nodes.items():
        child_refs: list[str] = []
        for dep_id in node.get("dependencies", {}):
            if dep_id in refs_by_id:
                child_refs.append(refs_by_id[dep_id])
        if child_refs:
            dependencies.append({
                "ref": refs_by_id[node_id],
                "dependsOn": sorted(set(child_refs)),
            })

    bom = {
        "$schema": "http://cyclonedx.org/schema/bom-1.4.schema.json",
        "bomFormat": "CycloneDX",
        "specVersion": "1.4",
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "version": 1,
        "metadata": {
            "component": {
                "type": "application",
                "bom-ref": root_ref,
                "name": consumer["name"],
                "version": consumer["version"],
                "purl": conan_purl(consumer["name"], consumer["version"]),
            },
            "tools": [
                {
                    "vendor": "Coditary",
                    "name": "beez-conan-sbom",
                    "version": "1",
                }
            ],
        },
        "components": components,
        "dependencies": dependencies,
    }

    consumer_licenses = license_entries(consumer)
    if consumer_licenses:
        bom["metadata"]["component"]["licenses"] = consumer_licenses

    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(bom, handle, indent=2)
        handle.write("\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
