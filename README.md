# DOWNFALL

An original multiplayer urban operations game targeting **Unreal Engine 5.8**.
Four responders enter **Breakwater**, a district of the fictional coastal city
**Velmora**, to restore a relay while damaged municipal robots control the streets.
The long-term design expands into combined-arms battles across a connected region.

**Current delivery: source prototype, not a compiled or photorealistic game.**
The project contains C++ gameplay, an automatically generated district, original
procedural surface textures and articulated model kits. The user confirmed a
successful native build at `9bef37b` with UE 5.8 / MSVC 14.44. The subsequent scene
update still requires a local engine build and visual review; Unreal is absent
from the cloud workspace. Production meshes, skeletal animation, dialogue
recordings and sound assets are not bundled. See [scene update and black-screen
recovery](Docs/SceneUpdate.md).

## Start here

1. Install a licensed **UE 5.8** distribution and its supported C++ toolchain.
   A source engine build with server support is needed for `DownfallServer`.
2. Set `UE_ROOT` to the folder containing `Engine/` and run, from this checkout:

   ```sh
   python Scripts/build.py --project-files
   python Scripts/build.py --target editor
   ```

3. On Windows, **LaunchDownfall.bat** builds and opens the editor. First interactive
   launch automatically generates `/Game/Downfall/Maps/L_Breakwater_District`.
   Wait for generation and material shaders. If automatic setup is disabled,
   run `Scripts/build_vertical_slice.py` through **Tools → Execute Python Script**.
   Existing maps are protected. See [tooling instructions](Scripts/README.md).
4. Start the first separate-process multiplayer test:

   ```sh
   python Scripts/run_multiplayer.py --clients 2
   ```

5. Perform [multiplayer acceptance](Docs/MultiplayerAcceptance.md), then repeat
   with four clients and cooked game/server targets. A launcher completing is
   not evidence that the gameplay tests passed.

No project-specific account, service key or asset purchase is needed for the
primitive blockout. Use the existing checkout; cloud tasks are already isolated.

## Native systems

| Source | Delivered behavior; engine validation pending |
|---|---|
| `Core/` | GameInstance, GameMode, GameState, PlayerController, PlayerState; four-player co-op policy and respawn |
| `Character/` | Enhanced Input, third-person shoulder camera, walking/jumping/crouching, predicted sprint, health/combat composition |
| `Combat/` | Kestrel C6 automatic rifle, Vesper P12 sidearm; server projectiles, finite ammunition, reload, cadence/request limits and team damage rules |
| `Vehicle/` | One-seat utility vehicle, validated entry/exit, server kinematic movement and replicated snapshots; no Chaos suspension or client prediction |
| `AI/` | Sentinel R3 primitive robot: patrol, visible-target pursuit, last-seen search, line-of-sight fire and sensor-region damage multiplier |
| `World/` | Server clock/weather transitions, client lighting/fog, relay capture and persistent destructible-cover states |
| `Presentation/` | Native HUD, optional 12-second camera/subtitle rehearsal, asset-driven positional audio API and acoustic boxes |

The map generator assembles a 180 × 180 m district with four enterable ground
floors, detailed façade/roof kits, street furniture, textured weather-responsive
surfaces, two roads, four player starts, four breakable barriers, one robot, one
vehicle and a relay. This is a small non-partitioned prototype with procedural
geometry. The full World Partition city is a production milestone.

## Controls

| Input | Action |
|---|---|
| WASD / mouse | Move / aim |
| Left mouse / R | Fire / reload |
| Q or wheel up | Switch weapon |
| Shift / Ctrl / Space | Sprint / crouch / jump |
| V | Switch camera shoulder |
| E | Enter nearby vehicle; exit when nearly stopped and the exit is clear |
| Vehicle WASD / Space | Throttle, reverse, steering / brake |
| F8 / F9 | Play / skip local cinematic rehearsal; use standalone clients to avoid PIE shortcut conflicts |
| Escape | Release / recapture mouse; the server continues running |
| F1 | Show / hide field controls |

## Production handoff

- [Production bible](Docs/ProductionBible.md): concept, visual identity, modes, regional map and phase gates.
- [Architecture](Docs/Architecture.md): exact folder conventions, authority boundaries, network and scale migration.
- [Vertical slice and first cinematic](Docs/VerticalSlice.md): scene layout, mission flow, shot list, dialogue and asset integration.
- [Art, animation, materials and audio](Docs/ArtAndPerformancePipelines.md): production recipes, asset budgets, accessibility and profiling.
- [Asset manifest](Content/Downfall/AssetManifest.json): 20 original model specifications with dimensions, topology, UVs, rigs, collision, LODs and damage.
- [Feature status](Docs/FeatureStatus.md): IMPLEMENTED / PROTOTYPED / PLANNED / PLACEHOLDER distinctions.
- [Validation evidence](Docs/Validation.md): checks actually executed and remaining engine gates.

Portable checks available without Unreal:

```sh
python Scripts/validate_assets.py
python -m unittest discover -s Tests -v
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -I Source/Downfall/Public -x c++ Source/Downfall/Private/Combat/DownfallCombatRules.portable.inl -o /tmp/downfall-combat-rules
/tmp/downfall-combat-rules
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -I Source/Downfall/Public -x c++ Source/Downfall/Private/World/DownfallWorldMath.portable.inl -o /tmp/downfall-world-math
/tmp/downfall-world-math
```

These execute the same portable combat and environmental math used by native
gameplay; they do not replace Unreal networking, rendering or integration tests.
