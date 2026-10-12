# Tooling and truthful validation

These scripts target a separately installed, licensed Unreal Engine **5.8**.
`UE_ROOT` is the directory that contains `Engine/`. A source engine build with
dedicated-server targets is required for the server workflow. No engine binary,
production mesh, motion capture, voice recording or cooked game is included.

Run from the repository root, preserving the existing checkout. Each cloud task
is already isolated; creating a Git worktree is unnecessary unless requested.

```sh
python Scripts/validate_assets.py
python -m unittest discover -s Tests -p 'test_*.py'
python -m compileall -q Scripts Tests
```

These checks validate the JSON contracts and Python tooling. They cannot establish
that Unreal C++ builds, that the map opens, or that multiplayer gameplay works.

## Build the editor

Generate IDE files without relying on Windows file associations:

```sh
python Scripts/build.py --project-files
```

On Windows, when `UE_ROOT` is not set, provide the installed engine directly:

```powershell
py Scripts\build.py --project-files --engine-root 'C:\Program Files\Epic Games\UE_5.8'
```

Linux example:

```sh
export UE_ROOT=/path/to/UnrealEngine-5.8
python Scripts/build.py --target editor
```

PowerShell example:

```powershell
$env:UE_ROOT = 'D:\UnrealEngine-5.8'
python Scripts/build.py --target editor
```

`build.py --dry-run` validates engine version and paths and prints argument arrays.
It does not compile. Build execution uses subprocess argument arrays, without a
shell command string. The launcher builds the native host platform. Configure
cross-compilation in a separate, platform-validated build pipeline.

## Generate Breakwater

After a successful native editor build, open `Downfall.uproject`. On Windows,
`LaunchDownfall.bat` performs both steps. The first interactive launch generates
the missing district automatically; later launches preserve it. For manual
generation, save or discard all unsaved editor work, then execute
`Scripts/build_vertical_slice.py` using **Tools > Execute Python Script**.
The project must enable Editor Scripting
Utilities and Python Editor Script Plugin. Equivalent Linux invocation:

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PWD/Downfall.uproject" \
  -ExecutePythonScript="$PWD/Scripts/build_vertical_slice.py" -unattended
```

Equivalent PowerShell invocation:

```powershell
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\Downfall.uproject" `
  "-ExecutePythonScript=$PWD\Scripts\build_vertical_slice.py" -unattended
```

Use a GPU-capable editor session for visual review. This script uses actual
Editor actor/material APIs and must be executed against UE 5.8 to validate those
APIs; standalone Python syntax validation is insufficient.

The generator produces `/Game/Downfall/Maps/L_Breakwater_District`, procedural PBR
materials, four PlayerStarts, a 180 m street district, four enterable ground floors,
façade/roof kits, furnishings and street props, lighting and native gameplay actors.
The expected file is `Content/Downfall/Maps/L_Breakwater_District.umap`. Original
packed texture sources are in `ArtSource/Surfaces`; built-in shapes form the kit
models. Production model fidelity remains **PLACEHOLDER**. See
[scene update](../Docs/SceneUpdate.md) for local acceptance and limitations.

An existing map is protected by default. An explicit rebuild requires:

```sh
export DOWNFALL_REBUILD_SLICE=1
```

or `$env:DOWNFALL_REBUILD_SLICE = '1'` in PowerShell before editor launch. Rebuild
replaces only actors with `Downfall.Generated.Blockout`. Untagged actors and
existing materials are retained. Editing a tagged generated actor means that
edit will be replaced on explicit rebuild. The generator saves the map and a
report at `Saved/BreakwaterBuildReport.json`; review Unreal's Output Log for
errors before attempting a cook.

## Build and stage gameplay

```sh
python Scripts/build.py --target server
python Scripts/build.py --target game
python Scripts/build.py --target server --cook
python Scripts/build.py --target game --cook
```

Cook requires the generated map to exist. BuildCookRun uses the named map and
does not launch, upload or publish a build. Staged output uses Unreal's normal
`Saved/StagedBuilds` locations. Multiplayer launch and acceptance instructions
are provided by `run_multiplayer.py` and the project documentation.

## Asset manifest

`Content/Downfall/AssetManifest.json` contains twenty original production asset
specifications, each explicitly **PLANNED**, including dimensions, triangle
budgets, topology, Nanite choice, UVs, texture channels, skeletons, collision,
LODs, damage states and animation requirements. These are specifications for
art production, not delivered production assets. The validator rejects invalid
budgets, duplicate asset identities, missing PBR channels and malformed fields.
