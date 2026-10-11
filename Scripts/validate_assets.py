#!/usr/bin/env python3
"""Validate original production asset specifications without requiring Unreal.

This validates budgets and authoring contracts, not the existence or fidelity of
the planned assets. Run from any directory: python Scripts/validate_assets.py.
"""
from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path

STATUSES = {"IMPLEMENTED", "PROTOTYPED", "PLANNED", "PLACEHOLDER"}
CATEGORIES = {"character", "robot", "vehicle", "weapon", "environment", "prop", "vegetation"}
BASE_CHANNELS = {"base_color", "normal", "roughness", "ambient_occlusion"}


def validate_manifest(manifest: object) -> list[str]:
    errors: list[str] = []
    if not isinstance(manifest, dict):
        return ["Manifest must be an object"]
    if manifest.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    if manifest.get("units") != "meters":
        errors.append("units must be meters")
    if not manifest.get("engine_target"):
        errors.append("engine_target is required")
    assets = manifest.get("assets")
    if not isinstance(assets, list) or not assets:
        return errors + ["assets must be a non-empty array"]
    identifiers: set[str] = set()
    paths: set[str] = set()

    def positive_number(value: object) -> bool:
        return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) and value > 0

    for index, asset in enumerate(assets):
        label = f"assets[{index}]"
        if not isinstance(asset, dict):
            errors.append(f"{label}: must be an object")
            continue
        identifier = asset.get("id")
        if not isinstance(identifier, str) or not re.fullmatch(r"[A-Z][A-Za-z0-9_]+", identifier):
            errors.append(f"{label}: id must be a stable alphanumeric asset identifier")
        elif identifier in identifiers:
            errors.append(f"{label}: duplicate id {identifier}")
        else:
            identifiers.add(identifier)
            label = identifier
        if asset.get("status") not in STATUSES:
            errors.append(f"{label}: invalid status")
        if asset.get("category") not in CATEGORIES:
            errors.append(f"{label}: invalid category")
        path = asset.get("content_path")
        if not isinstance(path, str) or not re.fullmatch(r"/Game/Downfall/[A-Za-z0-9_/]+", path):
            errors.append(f"{label}: invalid content_path")
        elif path in paths:
            errors.append(f"{label}: duplicate content_path")
        else:
            paths.add(path)
        dimensions = asset.get("dimensions_m")
        if not isinstance(dimensions, list) or len(dimensions) != 3 or not all(positive_number(v) for v in dimensions):
            errors.append(f"{label}: dimensions_m must contain three finite positive numbers")
        topology = asset.get("topology", {})
        if not isinstance(topology, dict):
            topology = {}
        triangles = topology.get("triangles_lod0")
        if not isinstance(triangles, int) or isinstance(triangles, bool) or triangles <= 0:
            errors.append(f"{label}: topology.triangles_lod0 must be a positive integer")
        if not topology.get("authoring_notes"):
            errors.append(f"{label}: topology.authoring_notes required")
        nanite = asset.get("nanite", {})
        if not isinstance(nanite, dict) or not isinstance(nanite.get("enabled_target"), bool) or not nanite.get("reason"):
            errors.append(f"{label}: nanite requires enabled_target and reason")
        rigging = asset.get("rigging", {})
        if not isinstance(rigging, dict) or rigging.get("type") not in {"none", "skeletal", "rigid_parts"}:
            errors.append(f"{label}: rigging.type invalid")
            rigging = {}
        if rigging.get("type") == "skeletal" and not rigging.get("skeleton"):
            errors.append(f"{label}: skeletal assets require skeleton")
        uv = asset.get("uv", {})
        if not isinstance(uv, dict) or not uv.get("layout") or not positive_number(uv.get("texel_density_px_per_meter")):
            errors.append(f"{label}: uv requires layout and positive texel_density_px_per_meter")
        texture_sets = asset.get("texture_sets")
        if not isinstance(texture_sets, list) or not texture_sets:
            errors.append(f"{label}: texture_sets required")
        else:
            for texture in texture_sets:
                if not isinstance(texture, dict):
                    errors.append(f"{label}: texture set must be an object")
                    continue
                resolution = texture.get("source_resolution")
                if not isinstance(resolution, int) or isinstance(resolution, bool) or resolution < 256 or resolution > 8192 or resolution & (resolution - 1):
                    errors.append(f"{label}: source_resolution must be a power of two in [256, 8192]")
                channels = texture.get("channels")
                if not isinstance(channels, list) or not all(isinstance(v, str) for v in channels) or not BASE_CHANNELS.issubset(channels):
                    errors.append(f"{label}: texture set missing base PBR channels")
                if not isinstance(texture.get("streaming"), str) or not texture["streaming"]:
                    errors.append(f"{label}: texture set streaming policy required")
        for field in ("materials", "collision", "damage_states", "animations"):
            if not isinstance(asset.get(field), list) or not asset[field] or not all(isinstance(v, str) and v for v in asset[field]):
                errors.append(f"{label}: {field} must be a non-empty list of descriptions")
        lod = asset.get("lod", {})
        ratios = lod.get("triangle_ratios") if isinstance(lod, dict) else None
        if not isinstance(ratios, list) or not ratios or ratios[0] != 1.0 or not all(positive_number(v) and v <= 1 for v in ratios) or any(a <= b for a, b in zip(ratios, ratios[1:])):
            errors.append(f"{label}: lod.triangle_ratios must start at 1 and decrease strictly")
        if not isinstance(lod, dict) or not lod.get("strategy"):
            errors.append(f"{label}: lod.strategy required")
        if not isinstance(asset.get("source"), str) or not asset["source"]:
            errors.append(f"{label}: source required to distinguish generated blockout from production assets")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", nargs="?", type=Path, default=Path(__file__).resolve().parents[1] / "Content/Downfall/AssetManifest.json")
    args = parser.parse_args()
    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"FAIL: {exc}")
        return 2
    errors = validate_manifest(manifest)
    for error in errors:
        print(f"FAIL: {error}")
    if errors:
        return 1
    print(f"PASS: {len(manifest['assets'])} asset specifications; asset fidelity and engine import are not validated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
