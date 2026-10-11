# Delivery status

Status applies to the artifact, not the advertised long-term game. **IMPLEMENTED**
means executable code exists and the named standalone checks passed.
**PROTOTYPED** means source for a limited behavior exists, with Unreal build/runtime
validation pending. **PLANNED** means a design or asset specification only.
**PLACEHOLDER** means intentional temporary representation.

No Unreal-dependent feature is marked engine-verified. No measured frame rate,
player capacity, cooked build or photorealistic image is available yet.

| Area | Status | Evidence / remaining work |
|---|---|---|
| Downfall concept, Velmora, Breakwater | PLANNED | Original production bible and concrete map/mission conventions |
| Asset manifest validation | IMPLEMENTED | 20 schema-valid specifications, including invalid-input unit tests; does not validate meshes |
| Portable combat rules | IMPLEMENTED | C++17 compiled with warnings-as-errors and UBSan; ammunition, cadence, request budget and damage permissions passed |
| Portable clock/solar math | IMPLEMENTED | C++17/UBSan tests passed; solar-time approximation and artistic lunar phase, not full ephemerides |
| Build/editor/server targets | PROTOTYPED | `.uproject`, Build.cs, three targets and build launcher; engine compile unrun |
| Separate-process multiplayer launcher | IMPLEMENTED tooling | Command/validation tests passed; actual Unreal processes unrun |
| Breakwater generator | PROTOTYPED | Real Editor Python actor/material/save calls; engine execution unrun |
| City/buildings/characters/weapons/vehicle models | PLACEHOLDER | Built-in primitives assembled by code; four blockout interiors, no production asset fidelity |
| Four-player co-op session | PROTOTYPED | Ordinary replication, direct connect, team 1, configured four-player cap; live test pending |
| Character movement | PROTOTYPED | CharacterMovement and native Enhanced Input; sprint saved-move flag, crouch, jump, shoulder camera |
| Prone, lean, mantle, vault, climb, slide, swim, ladder, cover | PLANNED | Animation and server movement contracts; no motion assets or traversal implementation |
| First person / full-body animation | PLANNED | Third-person prototype only; no skeletal animation assets |
| Health/death/respawn | PROTOTYPED | Server health component, team policy and five-second guarded respawn; primitive death pose |
| Two weapons / ballistics | PROTOTYPED | Server swept projectiles with gravity, reload, finite reserve, spread; presentation hooks |
| Armor, penetration, ricochet, suppression, injury, melee | PLANNED | No claims of full realistic combat or hit rewind |
| Inventory, loot, extraction economy | PLANNED | No persistence/backend or item system |
| Utility vehicle | PROTOTYPED | One seat, range/LOS entry, swept exit, server motion, explicit snapshots for driver and observers |
| Vehicle suspension, interior, deformation, mirrors, sound | PLANNED | Kinematic box prototype; no Chaos vehicle, prediction or articulated wheels |
| Aircraft, boats, motorcycles, tanks/APCs | PLANNED | Representative production asset specifications only |
| Sentinel robot | PROTOTYPED | Server bounded perception, patrol/pursuit/search/fire; primitive sensor-region damage bonus |
| Tactical squads, State Trees/BT/EQS/Smart Objects, crowds | PLANNED | No flanking/cover planner or civilian life simulation; simple robot has no navmesh pathfinding |
| Relay objective | PROTOTYPED | Server occupancy capture, progress decay, replicated ownership; robots do not contest it |
| Structural destruction | PROTOTYPED | Intact/damaged/destroyed barrier collision state and six local cosmetic fragments |
| Full Chaos building/vehicle destruction | PLANNED | No fracture collections, deformation assets, persistent world damage service or craters |
| Day/night and weather state | PROTOTYPED | Shared clock; clear/rain/storm/fog/snow transitions; directional light, skylight, fog and night lamps |
| Rain/snow effects, accumulation, droplets, puddle simulation | PLANNED | No Niagara weather systems or authored weather materials; wet-looking road is fixed placeholder roughness |
| Accurate moon/stars/tides/season ephemerides | PLANNED | Approximate phase/orbit only; no star field or astronomy accuracy claim |
| NPC schedules, traffic, dynamic businesses/grid | PLANNED | Design only |
| Local intro | PROTOTYPED | Twelve-second camera/subtitle rehearsal, F8/F9, local handoff; live match continues |
| Sequencer/facial dialogue/mocap/voice | PLANNED | Sixty-second shot script; no recorded performance or Level Sequence asset |
| Spatial audio API | PROTOTYPED | Asset-driven point sources, 343 m/s delay, bounded events, box zones and one-ray occlusion |
| Audible soundscape / MetaSounds / VO / score | PLANNED | No sound assets are bundled or connected; current scene is silent |
| HUD | PROTOTYPED | Native health/ammo/relay/time/subtitle display and projected aim reticle |
| CommonUI menus, settings/remapping/accessibility screens | PLANNED | Concrete UX specifications; no menus represented as implemented |
| Matchmaking, parties, voice, chat, reporting, friends/crossplay | PLANNED | Provider integration/authentication/platform agreements required |
| GAS | PLANNED integration | Plugin/module enabled; current health/weapon implementation does not use ASC/AttributeSets |
| Lumen/VSM/TSR | PROTOTYPED configuration | Project renderer defaults; no rendering or GPU performance validation |
| Nanite/HLOD/World Partition/OFPA/PCG/Mass | PLANNED | No production city or content import has exercised these systems |
| Hardware RT/Substrate/vendor upscalers/frame generation | PLANNED | Opt-in validation; RT off by default, Substrate/vendor plugins not enabled |
| Replication Graph / Iris | PLANNED alternatives | Baseline uses ordinary replication; migration follows measurements, not simultaneous assumed compatibility |
| 8/16/32/64+ players / massive battles | PLANNED | Four-player acceptance first; no scale or stability benchmark completed |
| Six graphics tiers / photo mode / path-traced replay | PLANNED | No graphics UI or capture feature; performance budget document only |
| QA/performance readiness | PARTIAL evidence | Standalone checks passed; UHT, build, cook, multiplayer, audio/render/animation and soak tests unrun |

The first playable **candidate** becomes a tested slice only after the engine
gates in [MultiplayerAcceptance.md](MultiplayerAcceptance.md). Its placeholders
must then be replaced and reviewed as a separate visual/audio production gate.
