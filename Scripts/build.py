#!/usr/bin/env python3
"""Build Downfall with a locally installed, licensed Unreal Engine 5.6.

Examples:
  python Scripts/build.py --target editor
  python Scripts/build.py --target server --configuration Development
  python Scripts/build.py --target game --cook --map /Game/Downfall/Maps/L_Breakwater
Set UE_ROOT to the directory containing Engine. UE binaries are not downloaded.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-root", type=Path, default=None)
    parser.add_argument("--target", choices=("editor", "game", "server"), default="editor")
    parser.add_argument("--configuration", choices=("DebugGame", "Development", "Shipping"), default="Development")
    parser.add_argument("--cook", action="store_true", help="Build, cook and stage; requires a generated and saved map")
    parser.add_argument("--map", default="/Game/Downfall/Maps/L_Breakwater")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    engine_input = args.engine_root or os.environ.get("UE_ROOT")
    if not engine_input:
        parser.error("UE_ROOT is unset. Point it at your licensed Unreal Engine 5.6 installation.")
    engine_root = Path(engine_input).expanduser().resolve()
    project = Path(__file__).resolve().parents[1] / "Downfall.uproject"
    version_path = engine_root / "Engine/Build/Build.version"
    if not version_path.is_file() or not project.is_file():
        parser.error(f"Missing Engine/Build/Build.version at {engine_root}, or project at {project}")
    try:
        version = json.loads(version_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        parser.error(f"Cannot read Unreal Engine version: {exc}")
    if (version.get("MajorVersion"), version.get("MinorVersion")) != (5, 6):
        parser.error("This prototype targets Unreal Engine 5.6. Validate an engine upgrade separately.")
    host = platform.system()
    if host not in {"Windows", "Linux", "Darwin"}:
        parser.error(f"Unsupported build host: {host}")
    target_platform = {"Windows": "Win64", "Linux": "Linux", "Darwin": "Mac"}[host]
    suffix = ".bat" if host == "Windows" else ".sh"
    batch = engine_root / "Engine/Build/BatchFiles"
    if args.cook:
        if args.target == "editor":
            parser.error("--cook requires --target game or server")
        if not args.map.startswith("/Game/Downfall/") or any(c not in "/_" and not c.isalnum() for c in args.map):
            parser.error("--map must be a /Game/Downfall/ package path")
        map_file = project.parent / "Content" / (args.map.removeprefix("/Game/") + ".umap")
        if not map_file.is_file():
            parser.error(f"Map does not exist: {map_file}. Run build_vertical_slice.py in Unreal Editor first.")
        executable = batch / ("RunUAT" + suffix)
        command = [str(executable), "BuildCookRun", f"-project={project}", "-noP4", "-utf8output", "-unattended", "-build", "-cook", "-stage", "-pak", f"-map={args.map}"]
        if args.target == "server":
            command += ["-server", "-noclient", f"-serverplatform={target_platform}", f"-serverconfig={args.configuration}"]
        else:
            command += [f"-platform={target_platform}", f"-clientconfig={args.configuration}"]
    else:
        build_folder = batch if host == "Windows" else batch / ("Linux" if host == "Linux" else "Mac")
        executable = build_folder / ("Build" + suffix)
        target = {"editor": "DownfallEditor", "game": "Downfall", "server": "DownfallServer"}[args.target]
        command = [str(executable), target, target_platform, args.configuration, f"-Project={project}", "-WaitMutex", "-NoHotReloadFromIDE"]
    if not executable.is_file():
        parser.error(f"Unreal build entry point not found: {executable}")
    print("Unreal build arguments:", json.dumps(command), flush=True)
    if args.target == "server":
        print("Dedicated-server targets require an engine build with server support (normally a source build).", flush=True)
    if args.dry_run:
        return 0
    try:
        return subprocess.run(command, cwd=project.parent, check=False).returncode
    except OSError as exc:
        print(f"Unable to start Unreal build: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
