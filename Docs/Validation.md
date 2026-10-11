# Validation evidence

Source-authoring environment: Linux, Python 3 and GCC available. Unreal Editor,
Unreal Build Tool and a configured `UE_ROOT` were absent.

| Check actually executed | Result |
|---|---|
| `python3 -m unittest discover -s Tests -v` | 13 passed: asset schema rejection cases and multiplayer-launcher arguments/readiness parsing |
| `python3 Scripts/validate_assets.py` | PASS: all 20 original production specifications satisfy the manifest contract |
| Python byte compilation, project JSON, reflected-header include order, documentation links and whitespace | PASS; these are source checks, not Unreal API/build validation |
| Portable combat C++ compiled with `-std=c++17 -Wall -Wextra -Werror -fsanitize=undefined` | PASS: 4,080,300 ammunition-conservation cases; firing cadence under repeated requests, request-token budget, friendly/self/environmental damage, NaN and overflow guards; no UBSan diagnostic |
| Portable world C++ with the same compiler flags | PASS: 86,400 clock/reanchor cases, 1,568,040 bounded solar positions, equinox/noon/sunrise/season and approximate moon-phase checks; no UBSan diagnostic |
| `python3 Scripts/build.py --target editor` | BLOCKED before engine invocation: `UE_ROOT` is unset and UE 5.8 is not installed |

The C++ portable harnesses include the same rules/math headers used by gameplay.
They do **not** compile UObject classes, generated headers, RPCs, physics or
rendering. Python manifest tests verify specifications, not 3D models or art.

Source integration review corrected: character visibility trace collision,
projectile-specific collision channel, vehicle snapshots reaching its owning
client, abandoned-seat cleanup, swept exit clearance, request throttling,
camera-relative HUD aiming, and documentation/API naming mismatches. These
corrections still require engine-side regression testing.

**UNRUN:** UHT/native project compilation; Editor Python API execution; generated
map inspection; all eight Unreal automation tests; live two/four-client dedicated
server tests; cook/package; audio/visual/animation review; bandwidth/load/soak;
hardware RT, Lumen and other renderer validation. There is no generated `.umap`,
`.uasset`, cooked executable or rendered beauty capture in this delivery.

The next prerequisite is a licensed UE 5.8 installation with the supported native
toolchain. Set `UE_ROOT`, build the editor, generate the scene, and follow
[MultiplayerAcceptance.md](MultiplayerAcceptance.md). Review compile/API errors
before declaring the first playable candidate ready.
