# First multiplayer gameplay test

**Current result: UNRUN.** Unreal Engine is unavailable in the source-authoring
environment. This procedure defines the first acceptance run; it is not a passing
test report. Use a compiled UE 5.6 editor and generated `L_Breakwater` first.

## Native automation

After `python Scripts/build.py --target editor`, run the eight named automation
tests under `Downfall.Combat` and `Downfall.World`. For example, on Linux:

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Downfall.uproject" \
  -unattended -nop4 -nullrhi \
  '-ExecCmds=Automation RunTests Downfall.' \
  '-TestExit=Automation Test Queue Empty' \
  "-ReportExportPath=$PWD/Saved/Automation"
```

Inspect this run's timestamped results, test names and counts. Require **8 passed,
0 failed** and no unexpected skips; a successful process or empty report does
not pass. These native tests exercise rules/math, not multiplayer RPC execution.
Record engine changelist, compiler, platform, project revision and command.

## Two separate clients and a dedicated editor server

Run `python Scripts/run_multiplayer.py --clients 2`. It starts a headless editor
server, waits for the network-driver readiness log, then starts separate client
processes. UDP port 7777 is local by default. Logs and `run.json` are saved in
`Saved/MultiplayerTests/<timestamp>/`. The report deliberately leaves gameplay
acceptance `UNRUN`; fill out the following observations manually.

| ID | Action | Required observation |
|---|---|---|
| MP01 | Join A and B, walk around each other, sprint/crouch/jump | Distinct possessed pawns; matching transforms and states; no shared input/window ownership |
| MP02 | A fires Kestrel at barrier, B observes; reload, switch to Vesper, release/repress quickly | Server consumes ammunition; correct reserve transfer; visible projectile/impact behavior; repeated trigger cannot exceed cadence |
| MP03 | A shoots B, then the robot | Co-op team damage rejected; enemy takes damage, upper capsule hits get the intended 1.5× bonus; no client-supplied damage path |
| MP04 | Let robot see A; duck behind solid cover, then move while hidden | Robot fires only with line of sight and range; follows last seen location, not hidden current position; A loses health and respawns after death |
| MP05 | A enters vehicle, B observes; drive and brake | Both A and B receive movement snapshots; original body is hidden/attached; only current owner drives; prototype latency/jitter is recorded |
| MP06 | Attempt exit beside wall, then move to open ground and exit | Exit cannot pass through wall or place capsule inside obstruction; character input returns; vehicle input context is removed |
| MP07 | Disconnect A while driving, reconnect | Seat becomes available; no stranded hidden body/active throttle; reconnect spawns a new eligible player |
| MP08 | Damage then destroy a barrier | Both clients agree on silhouette and collision; walking/projectiles pass through destroyed portion; cosmetic fragments never block players |
| MP09 | Enter relay region for 15 seconds, leave before completion once, then finish | Progress increases server-side, decays when empty, and ends in team-1 ownership; HUD says RELAY ONLINE; no mission-end screen is expected |
| MP10 | Compare clocks; wait through weather change at 120 real seconds | Clocks converge under GameState synchronization; blend is continuous; lights/fog agree; no rain particles or audio are expected without assets |
| MP11 | A plays/skips F8/F9 rehearsal while B keeps moving/firing | Only A's camera changes; subtitle timing runs; skip/end returns camera; B's input/match continues |
| MP12 | Rejoin after capture and destruction | Captured relay and destroyed collision state reconstruct without replaying old cosmetic events |
| MP13 | Repeated respawn/enter/exit/intro cycles; travel or close client mid-intro | No retained cameras/input locks, duplicate contexts, crashes, stuck seat or ongoing old fire timer |

Run the same list with four clients. Then try a fifth connection and verify it is
rejected with no capacity race or half-created pawn. Four editor windows are not
a server performance benchmark. Robot-contested objectives are not implemented;
competitive team contest requires an authority-only team fixture and a separate
test branch rather than client-editable team IDs.

## Network adversity and security gates

On a development test instance, run Unreal's supported packet simulation console
commands `Net PktLag=100`, `Net PktLagVariance=20`, `Net PktLoss=2` and repeat
MP02/05/06/09/11/12. Record where the commands are applied; applying delay at both
ends doubles the added path delay. Restore values to zero before baseline runs.
Log corrections, dropped cosmetic events, join time and bytes/client. Vehicle
prediction and projectile visual smoothing are absent and must not be disguised
as completed latency handling.

Use an instrumented development client/automation fixture for invalid requests:
fire while dead, dry magazine, rapid equip/reload, interaction beyond 350 cm,
vehicle input containing NaN/infinity or excess magnitude, repeated exit requests,
requests sent from a non-owning actor, and stale ownership after dismount. Assert
server health/ammo/seat/pose state directly. The portable rule tests cover only
math; RPC ownership/rate correctness requires this real network fixture.

## Cooked-server gate

Using an engine distribution with server support:

```sh
python Scripts/build.py --target server --cook
python Scripts/build.py --target game --cook
```

Launch the staged server with `L_Breakwater -log -port=7777` and staged clients
with `127.0.0.1:7777 -log`. Select the actual staged executables reported by UAT
for the host platform; the editor launcher is not a packaged-build launcher.
Repeat all acceptance observations and inspect missing-asset/NetGUID/UHT/cook
warnings. Keep headless crash symbols and logs with the tested revision.

Next gates: ten-minute four-player route, one-hour reconnect/respawn soak,
server/client Unreal Insights traces, visual replacement pass and audible sound
validation after real sound assets are connected. The 16/32/64-player targets
remain unvalidated until separate load and adversarial suites pass.

## Recording results

Create a run report beside the logs with `build`, `engine`, `hardware`, `settings`,
`clients`, `packet_simulation`, `test_id`, `outcome`, `observed`, `evidence_paths`.
Use PASS / FAIL / BLOCKED / UNRUN separately; attach screenshots or captures only
from the tested executable. Open ports, live PIDs, design documents and portable
tests cannot establish the gameplay observations above.
