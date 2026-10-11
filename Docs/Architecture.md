# Unreal architecture and network contract

Target **Unreal Engine 5.6**, C++ runtime module `Downfall`, desktop development first. Engine-side compilation remains necessary. The initial foundation intentionally has one runtime module; the boundaries below define ownership without requiring premature plugins. Native blockout content is a test fixture for gameplay, not a substitute for art assets.

## Source and content organization

```text
TheDownfall/
  Downfall.uproject
  Config/
    DefaultEngine.ini
    DefaultGame.ini
    DefaultInput.ini
  Source/
    Downfall.Target.cs
    DownfallEditor.Target.cs
    DownfallServer.Target.cs
    Downfall/
      Downfall.Build.cs
      Public/
        Core/          # game mode/state, player state/controller, game instance
        Character/     # character, movement and health-facing composition
        Combat/        # authoritative weapons, health/damage, breakables
        World/         # replicated environment and blockout construction
        AI/            # native robot prototype; controller/perception integration
        Vehicle/       # authoritative ground vehicle prototype
        Presentation/  # HUD, local intro and environmental sound hooks
      Private/
        Core/
        Character/
        Combat/
        World/
        AI/
        Vehicle/
        Presentation/
  Content/Downfall/
    Maps/
    Characters/
      Humans/{Meshes,Materials,Textures,Rigs,Animations,Blueprints}/
    AI/Robots/{Meshes,Materials,Textures,Rigs,Animations,Blueprints}/
    Props/{Meshes,Materials,Textures}/
    Weapons/{Data,Meshes,Materials,Animations,Audio,FX}/
    Vehicles/{Data,Meshes,Materials,Animations,Audio,Blueprints}/
    Environment/
      Breakwater/{Architecture,Interiors,Street,Props,Lighting}/
      Shared/{Landscape,Foliage,Decals,Water,Weather}/
    Materials/{Functions,Masters,Instances,Collections}/
    FX/{Niagara,Textures,Materials}/
    Audio/{MetaSounds,Submixes,Attenuation,Reverb,Dialogue,Music}/
    Cinematics/LastLight/{Sequences,Cameras,Performances}/
    UI/{Screens,Widgets,Styles,Icons,Fonts}/
    Input/{Actions,Contexts}/
    Data/{Missions,Objectives,Factions,Scalability}/
    Developer/Validation/
  Docs/
  Scripts/
  Tests/
```

The source directories above are the implementation organization; content subdirectories are the exact production destination convention and may remain empty until editor-authored assets exist. Unreal `.uasset`/`.umap` files must be created by supported engine/editor tooling. Text pretending to be one of these binary formats is not an asset.

Use `Downfall` after Unreal native type prefixes (`ADownfallCharacter`, `UDownfallCombatComponent`); `BP_DF_`, `WBP_DF_`, `DA_DF_`, `IA_DF_`, `IMC_DF_`, `M_DF_`, `MI_DF_`, `MF_DF_`, `MPC_DF_`, `NS_DF_`, `MS_DF_`, `LS_DF_`, `SK_DF_`, `SM_DF_`, `PHYS_DF_`, and `ABP_DF_` for authored assets. Asset identifiers remain stable when display names change. Source filenames are the authority for actual native class names.

### Runtime ownership

| Owner | Lifetime and authoritative responsibility |
|---|---|
| GameInstance | Per-process session lifecycle and local settings; never the authoritative match state |
| GameMode | Server-only login/spawn policy, team assignment, mission transitions and win/loss decisions |
| GameState | Replicated operation ID, capacity and server-time reference; objectives and weather currently own their replicated state |
| PlayerController | Owning connection, input requests, local UI and local camera; server validation of player actions |
| PlayerState | Replicated identity within the match, team/squad and score; authenticated identity integration later |
| Character | Replicated movement, physical presence, owned combat/health components and equipment attachment |
| Health component | Server mutates health/death state; clients consume replication and presentation events |
| Combat/weapon component | Server validates equip/fire/reload requests, cooldown, magazine and hit resolution |
| Environment authority | Server selects time/weather target and transition; clients derive smooth presentation from a shared epoch |
| Robot | Server target selection, movement/damage and attack scheduling; client visual/audio playback |
| Vehicle | Server occupancy and physical/gameplay state; client requests from verified occupant only |
| Presentation | Local HUD, camera, sound and effects; cosmetic events cannot grant damage, health or inventory |

Data assets should replace hard-coded prototype tuning once native behavior is proven. Required schemas: `WeaponDefinition` (stable ID, projectile/trace policy, fire interval, magazine, reload duration, recoil curve, impact table), `RobotArchetype` (parts, resistances, sensors, attacks), `VehicleDefinition` (seats, movement envelope, damage zones), `WeatherProfile` (cloud/rain/snow/fog/wind/lightning ranges), and `MissionDefinition` (objective dependencies, permissions, encounter references). Validate ranges, missing references, duplicate IDs, and server-required assets in editor/cook automation.

### Plugin boundaries after the slice

Move tested responsibilities into `DownfallCore`, `DownfallCombat`, `DownfallVehicles`, `DownfallAI`, `DownfallWorld`, `DownfallPresentation`, and `DownfallOnline` only after their dependencies stabilize. Core defines lightweight types/interfaces and must not depend on presentation or vendor SDKs. Editor-only generation, validators, import automation and asset tooling belong in a separate editor module. Online provider code must sit behind an interface so authentication and session services can be changed without touching weapons.

## Network baseline

The initial code uses Unreal's ordinary actor/component replication and RPC system. **Replication Graph and Iris are architectural options, not implemented scale claims.** Character movement uses Unreal's movement replication where implemented. Project-specific collision, teleport, movement-mode, acceleration and vehicle handoff validation still require adversarial tests.

| Action | Request and server checks | Replication/result |
|---|---|---|
| Fire | Owned pawn/controller; alive; valid weapon; ammo; monotonic sequence if used; cooldown; finite aim; bounded aim divergence; legal origin | Server resolves damage; ammo owner-only where practical; compact cosmetic event to relevant observers |
| Reload | Owner/alive/compatible weapon; reserve and capacity; not already reloading | Server completion time and magazine; cancellation on death/equip/state changes |
| Interact | Owned pawn; known object; range; line of sight; mission/seat state; rate cap | Server mutates objective/door/seat; clients receive resulting state |
| Damage | Produced only by server-authorized gameplay | Health and death replicated; cosmetic hit information has no authority |
| Vehicle input | Sender is current authorized driver; finite bounded controls; rate cap; movement envelope | Authoritative transform/velocity and seat occupancy; correction/interpolation policy |
| Robot attack | Server sensor/target/range/cooldown/line-of-sight decision | Authoritative health result plus relevant cosmetic event |
| Weather/time | Server mission policy or authenticated administrator | Replicated epoch/profile/transition parameters; local deterministic interpolation |
| Mission/cinematic | Server mission transition; late-join policy; combat/control constraints | Replicate phase and sequence start timestamp; clients can skip local presentation without advancing mission |

These checks define the production contract; inspect the source/status matrix to see which are already present. A reliable RPC is not authentication or input validation. Avoid reliable RPCs every frame and unbounded multicast FX. Prefer state replication for durable outcomes and bounded unreliable cues for effects that can be dropped. Late joiners require state, not a replay of the entire event history.

Do not trust client-reported hit actor, damage amount, inventory quantity, completion state, world-space shot origin, or vehicle ownership. For the first projectile/trace prototype, resolve from a server-known muzzle/eye origin. Server rewind is future work: keep a bounded history, bound rewind by measured latency, use original server-time shot constraints, and define cover/door/destruction rewinds. Never simply accept an arbitrary client timestamp.

### Scale migration

Start with four clients; collect actor channel counts, bytes/client, serialization cost, relevant actor density, packet loss behavior, and server tick time. A candidate Replication Graph design routes players and vehicles spatially, squad and objectives via explicit policy, and stationary breakables through dormancy. Wake dormant actors before publishing state changes. Always-relevant actors must be few and small.

Evaluate Iris separately on a version-pinned branch against engine/plugin compatibility and actual migration cost. Do not assume a custom Replication Graph can be enabled unchanged with Iris; choose and validate one replication architecture for the shipping branch. Record correctness and bandwidth regressions before changing the baseline.

AI simulation LOD is independent of rendering LOD. Near combat uses full server AI; distant persistent groups use bounded tactical state and aggregate travel; ambient crowds can use Mass Entity client presentation only when they cannot influence authoritative outcomes. A crowd becoming combat-relevant must acquire a server-authoritative representation through a defined handoff. It must not spawn invulnerable cosmetic enemies that appear to be combatants.

### GAS integration plan

GAS is not claimed merely because its plugin or module is available. Migrate current health/ammo ownership intentionally: choose an authoritative AttributeSet for health/armor/stamina, map damage to Gameplay Effects, and remove or make read-only the old health storage in the same change. Two independent replicated health values are forbidden. Define prediction keys and reconcile predicted cosmetics; server owns damage and ability eligibility. Put player ASC on PlayerState only if persistence across pawn changes is required, with explicit avatar rebinding and reset tests.

## World, weather, and time contract

Authoritative environment state should contain an epoch server time, epoch solar minute, day length in real seconds, date/latitude/longitude for a calibrated solar model, weather profile IDs, blend start/end times, transition seed, and intensity scalars. Clients estimate current server time using GameState time sync and derive sky state locally. Never replicate every rain droplet or send directional-light transforms each frame.

The initial time/weather implementation may use simplified sun motion; physically correct solar/lunar ephemerides, lunar phases, eclipses, and seasonal stars remain planned until independently compared against reference values. Production solar tests need sunrise/noon/sunset at multiple latitudes and dates, including polar edge cases. A georeferenced city can use the engine Sun Position plugin or a verified solar algorithm, while moon/stars need their own validated data/model.

Weather presets: clear, light rain, heavy rain, thunderstorm, fog, dust, snow, blizzard, hail, and extreme storm. Each declares climate/season eligibility, minimum duration, transition envelope and gameplay effects. Avoid snow-to-dust transitions over minutes unless an authored incident motivates them. The initial profile set is only what source supports; do not claim all presets are simulated.

Material wetness integrates rainfall minus drainage/evaporation, with per-surface exposure and shelter masks. Puddle water level uses authored basin masks and bounded accumulation. Snow is a separate mass/coverage field driven by temperature, precipitation and melt; upward normal masking alone is a visual prototype. Foot/tire tracks and windshield droplets are local presentation with explicit memory and render budgets. Server gameplay traction/visibility uses coarse authoritative values independent of cosmetic resolution.

Lightning transmits a deterministic strike event (ID, position, time, energy class), not sound timing per client. Each client schedules thunder by distance/speed of sound and its acoustic environment. Hazardous strikes require a server damage query. Night windows and streetlights use thresholds with hysteresis, power-grid eligibility, and deterministic variation. NPC schedules, mission availability, business hours and tactics read authoritative world time, not the local computer clock.

## Dedicated server and services

Package a true `Server` target using a licensed engine distribution capable of server builds; a source-built Unreal engine is the usual reproducible path for this project. A successful editor listen-server run does not validate dedicated-server stripping, cooking, packaged asset references, Linux support, or headless performance. Pin engine revision, compiler/SDK, plugin versions, cook platform and content revision in CI artifacts.

The server build excludes local camera, rendering and audio work through explicit net-mode checks. CI runs UHT/build, cook, packaged startup, multi-client tests, and a clean shutdown. Keep server logs, traces, test summaries and crash symbols with the build identifier. Git LFS holds approved binary art; DerivedDataCache, Intermediate, Saved and local credentials are excluded.

Production service sequence: obtain platform/provider identity token → verify it server-side → request matchmaking/session allocation → receive short-lived connection authorization → join the allocated server → validate session membership → create PlayerState. TLS, short token lifetime, replay defenses, abuse rate limits, server secret isolation, and access-controlled telemetry are mandatory for an internet test. Do not put backend credentials in packaged clients or saved setup scripts.

Voice should use provider-authenticated channels, positional routing from server-authorized team/proximity membership, mute/block/report controls, and accessibility text alternatives. A player's cosmetic location cannot determine authorized voice membership. Recording and transcription require explicit policy/consent; no voice backend is implemented in the initial slice.
