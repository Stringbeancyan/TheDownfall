#!/usr/bin/env python3
"""Run one editor dedicated server and 2–4 separate graphical client processes.

This is a local multiplayer test launcher, not a claim that gameplay assertions pass.
Follow Docs/MultiplayerAcceptance.md during the run. Each process gets its own log.
Requires a compiled DownfallEditor, generated Breakwater map, and licensed UE 5.8.
"""
from __future__ import annotations

import argparse
import datetime
import json
import math
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import time


PROJECT_ROOT = Path(__file__).resolve().parents[1]
MAP = "/Game/Downfall/Maps/L_Breakwater"


def editor_executable(engine_root: Path, host: str) -> Path:
    locations = {
        "Windows": "Engine/Binaries/Win64/UnrealEditor.exe",
        "Linux": "Engine/Binaries/Linux/UnrealEditor",
        "Darwin": "Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor",
    }
    if host not in locations:
        raise ValueError(f"Unsupported host: {host}")
    return engine_root / locations[host]


def build_commands(editor: Path, project: Path, log_dir: Path, clients: int, port: int) -> list[list[str]]:
    if not 2 <= clients <= 4:
        raise ValueError("The first acceptance run supports 2–4 clients.")
    if not 1024 <= port <= 65535:
        raise ValueError("Port must be between 1024 and 65535.")
    common = [str(editor), str(project)]
    commands = [common + [MAP, "-server", "-unattended", "-nosound", "-nullrhi", "-log",
        "-NoSplash", f"-port={port}", f"-abslog={log_dir / 'server.log'}"]]
    for index in range(1, clients + 1):
        commands.append(common + [f"127.0.0.1:{port}", "-game", "-windowed", "-NoSplash", "-log",
            "-ResX=960", "-ResY=540", f"-WinX={20 + (index - 1) * 45}",
            f"-WinY={20 + (index - 1) * 45}", f"-abslog={log_dir / f'client-{index}.log'}"])
    return commands


def is_server_listening(log_path: Path, port: int) -> bool:
    try:
        text = log_path.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return False
    return re.search(rf"listening on port\s+{port}\b", text, re.IGNORECASE) is not None


def stop_processes(processes: list[subprocess.Popen]) -> None:
    # Only stop processes created by this invocation, in client-before-server order.
    for process in reversed(processes):
        if process.poll() is None:
            process.terminate()
    for process in reversed(processes):
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=10)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-root", type=Path)
    parser.add_argument("--clients", type=int, default=2)
    parser.add_argument("--port", type=int, default=7777)
    parser.add_argument("--duration", type=float, default=0, help="Seconds to run; 0 runs until Ctrl+C")
    parser.add_argument("--startup-timeout", type=float, default=120)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    root = args.engine_root or os.environ.get("UE_ROOT")
    if not root:
        parser.error("Set UE_ROOT to your licensed Unreal Engine 5.8 installation.")
    engine_root = Path(root).expanduser().resolve()
    try:
        editor = editor_executable(engine_root, platform.system())
        version = json.loads((engine_root / "Engine/Build/Build.version").read_text(encoding="utf-8"))
    except (ValueError, OSError) as exc:
        parser.error(str(exc))
    if (version.get("MajorVersion"), version.get("MinorVersion")) != (5, 8):
        parser.error("The project targets Unreal Engine 5.8.")
    if not editor.is_file():
        parser.error(f"UnrealEditor executable not found: {editor}")
    if not (PROJECT_ROOT / "Content/Downfall/Maps/L_Breakwater.umap").is_file():
        parser.error("Generate L_Breakwater with Scripts/build_vertical_slice.py in Unreal Editor first.")
    if not math.isfinite(args.duration) or args.duration < 0 or not 1 <= args.startup_timeout <= 600:
        parser.error("Duration must be nonnegative and startup timeout must be 1–600 seconds.")
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    log_dir = PROJECT_ROOT / "Saved/MultiplayerTests" / stamp
    try:
        commands = build_commands(editor, PROJECT_ROOT / "Downfall.uproject", log_dir, args.clients, args.port)
    except ValueError as exc:
        parser.error(str(exc))
    print(json.dumps(commands, indent=2), flush=True)
    if args.dry_run:
        return 0
    log_dir.mkdir(parents=True, exist_ok=False)
    processes: list[subprocess.Popen] = []
    handles = []
    exit_code = 0
    result = {"status": "STARTING", "gameplay_acceptance": "UNRUN", "commands": commands}
    try:
        for index, command in enumerate(commands):
            handle = (log_dir / f"process-{index}.stdout.log").open("w", encoding="utf-8")
            handles.append(handle)
            processes.append(subprocess.Popen(command, cwd=PROJECT_ROOT, stdout=handle, stderr=subprocess.STDOUT))
            if index == 0:
                deadline = time.monotonic() + args.startup_timeout
                while not is_server_listening(log_dir / "server.log", args.port):
                    if processes[0].poll() is not None:
                        raise RuntimeError("Server exited before its network driver became ready. Read server.log.")
                    if time.monotonic() >= deadline:
                        raise RuntimeError("Server network startup timed out. Read server.log; gameplay has not been tested.")
                    time.sleep(0.25)
        print(f"Processes launched. Logs: {log_dir}\nFollow Docs/MultiplayerAcceptance.md. Ctrl+C stops this run.", flush=True)
        started = time.monotonic()
        while args.duration == 0 or time.monotonic() - started < args.duration:
            for index, process in enumerate(processes):
                if process.poll() is not None:
                    raise RuntimeError(f"Process {index} exited during the run with code {process.returncode}; inspect its log.")
            time.sleep(0.25)
        result["status"] = "PROCESS_RUN_COMPLETED"
    except KeyboardInterrupt:
        result["status"] = "STOPPED_BY_OPERATOR"
    except (OSError, RuntimeError) as exc:
        result["status"] = "FAILED"
        result["error"] = str(exc)
        print(str(exc), file=sys.stderr)
        exit_code = 1
    finally:
        stop_processes(processes)
        for handle in handles:
            handle.close()
        (log_dir / "run.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("Gameplay acceptance is UNRUN until an operator records the checklist results.")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
