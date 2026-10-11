# Art, animation, materials, audio, and performance pipeline

This is an executable production specification: destinations, constraints, node contracts, review criteria, and measured-budget hypotheses. It does **not** supply scan libraries, digital humans, facial performances, rigs, Niagara assets, MetaSounds, or licensed middleware. The native scene uses basic geometry and procedural presentation until approved production assets replace them.

## Asset intake and ownership

Every asset has a stable identifier, owner, source file, import recipe, unit/axis convention, license/consent record, content destination, dependencies, collision policy, LOD policy and test scene. Record whether source is original, commissioned, licensed or generated. Do not ingest ripped game assets, logos, celebrity likenesses, unknown scans or training-source claims that cannot be substantiated. Generated output still needs topology, rig, UV, material, collision and rights review.

Author in meters or centimeters with explicit export units; Unreal receives centimeters, Z up, forward X. Apply transforms before export except deliberate skeletal bind transforms. Preserve deterministic skeleton names and socket transforms. Validate bounds, inverted normals, degenerate faces, UV coverage, material count, triangle count, texture dimensions, collision and asset dependencies in CI/editor validation. Texture suffixes: `_BC`, `_N`, `_ORM`, `_H`, `_ID`, `_E`; ORM packs occlusion/roughness/metallic only when the material contract agrees. Normal and mask maps use linear sampling; base color typically uses sRGB. Archive high-bit-depth source before runtime compression.

The asset manifest in the project is the machine-readable handoff when present; its schema/script validation is separate from Unreal import validation. A manifest row is a specification, not an existing model.

## Major-model matrix

Counts are starting **rendered triangle** budgets for LOD0 unless noted; they are review caps/hypotheses, not targets to fill. Dimensions are physical envelopes. Individual skeleton, collision and materials are still checked in engine. `H` = conventional skeletal human pipeline; `R` = robot rig; `S` = static module; `V` = moving vehicle as defined below.

| Asset / physical envelope | Geometry and Nanite | Materials / UV / source maps | Rig, collision, LOD and damage / animation |
|---|---|---|---|
| Mira, rescue systems engineer; 1.73 m tall | H; 90–140k body/clothes excluding hair, separate 45–80k head close-up; no dependency on experimental skeletal Nanite | Skin/eyes/teeth/tongue/hair/workwear; unique non-overlap head/body UV, reusable cloth trims; up to 8K head source, 4K body/clothes | Full facial/finger rig, correctives, cloth collars; capsule + authored physics asset; facial/hair/body LOD; wound decals; dialogue, locomotion, carry/revive |
| Orin, port electrician; 1.82 m | H, same shared skeleton and budgets; unique silhouette | 4K face/cloth gameplay, optional 8K close-up source; layered rain shell | Same rig contract; hearing aid/accessory sockets; hand tool gestures, exhausted speech, cover |
| Player responder; 1.65–1.95 m supported range | H; 70–120k full body, modular equipment; seam-safe deformation | 4K body/gear, shared tiling fabric; face detail only where visible | Size variants validated against collisions/doors; first/third-person hand alignment; gear anchors; damage masks, no hitbox advantage from cosmetics |
| Soldier / security NPC; 1.8 m | H; 60–100k | Four material slots desired, max eight justified; 4K kit atlas | Human skeleton; armor collision policy; combat, cover, surrender, injuries |
| Civilian adult; 1.55–1.95 m | H; 35–70k near, crowd LOD down to 5–12k | Shared 2–4K wardrobe atlases and skin variation with no duplicated whole sets per NPC | Reduced facial curves at distance, crowd locomotion; fleeing, phone, carrying bags, seated/sleep/work states |
| Sentinel R3 municipal robot; 2.2 × 1.4 × 1.1 m | R; 90–160k hard-surface, articulated pieces; conventional skeletal LOD baseline | Painted alloy/rubber/lens/cooling; 4K body + 2K tools; unique damage IDs, trim reuse | Separate sensors/tool/legs/power/cooling bones and hit zones; convex bodies, never full complex collision per joint; 4 LODs; disabled sensors/limbs, limp, brace, attack, collapse |
| Pocket reconnaissance drone; 0.55 × 0.55 × 0.18 m | R; 12–25k, rotor cards or low-poly blades | 2K atlas, emissive indicator mask | Root/gimbal/rotors; simple swept collision; sensor/body damage; hover, landing, damaged descent |
| Emergency utility truck; 5.1 × 2.0 × 2.15 m | V; 160–250k exterior + 100–180k interior; non-Nanite moving baseline | Up to 8K hero paint source, 4K interior, 2K glass/wheels; unique UV for decals/damage, tiled fabrics | Wheel/steering/suspension/door/wiper/seat bones; convex chassis and wheel queries; 5 LODs; glass, lamps, tires, hood and bounded damage morphs; enter/exit/drive |
| Civilian compact / sports coupe; 4.2–4.7 × 1.8–1.95 × 1.4–1.6 m | V; 120–220k exterior, 70–140k interior | 4K exterior and cockpit; shared tires/trim; metallic paint layered physically | V rig, front/rear seats; vehicle type handling data; wreck stage and localized panels; no generic mesh morph that moves gameplay collision invisibly |
| Motorcycle; 2.15 × 0.8 × 1.2 m | V; 70–120k | 4K hero atlas, 2K wheels; anisotropic metal | Steering/suspension/wheels, rider hand/foot IK; narrow collision and fall policy; lean/ride/dismount; deterministic wreck state |
| APC; 7.0 × 2.8 × 2.6 m | V; 220–350k exterior, 130–220k usable interior | 4K tile/trim, 4K unique markings/damage; 8K only for cinematic verified need | Turret/hatch/wheels/seats; hull collision plus independent hit zones; 5 LODs, armor/tool/wheel failure; crew poses and emergency egress |
| Tank; 8.2 × 3.5 × 2.7 m body | V; 250–400k; track animation budget explicit | 4K armor sets and trims, roughness breakup | Hull/turret/barrel/track/suspension; authoritative collision proxy; tracks/power/turret damage; recoil, hatch, crew |
| Utility helicopter; 13 × 3.2 × 4.1 m excluding full rotor span | V; 230–400k + 150k cockpit | 4K exterior/cockpit, 8K hero source justified by close shots | Rotor/gimbal/doors/gear; rotor visual representation by speed/LOD; hull sweep + rotor hazard policy; flight/damage/seat animation requires separate prototype |
| Patrol boat; 8.5 × 2.7 × 3.1 m | V; 160–260k + 100k cabin | 4K hull/cabin, wetness and salt layers | Rudder/prop/controls; buoyancy sample hull, simplified network physics; hull breach stages; steering/boarding |
| Coil carbine `C6 Kestrel`; 0.82 × 0.07 × 0.25 m | 35–65k first-person, 15–30k world; conventional skeletal | 4K first-person atlas + 2K accessories, unique UV for wear | Trigger/bolt/magazine/selectors; muzzle/eject/hand sockets; compact world collision; LODs; reload, inspect, fire, jam if designed |
| Sidearm `P12 Vesper`; 0.21 × 0.035 × 0.14 m | 18–35k first-person, 8–16k world | 4K source/2K runtime based on texel need | Slide/trigger/magazine; hand/muzzle/eject sockets; reload/fire/holster; no fantasy copied silhouette |
| Rescue kit / battery / helmet; 0.1–0.5 m | 3–18k each; static Nanite only for static placement where measured useful | 1–2K atlas or trims; readable unique labels | Attachment socket and pickup proxy; small-object collision disabled at distance; open/use/carry animations |
| Clinic facade bay; 4 × 0.35 × 3.6 m | S; 10–80k depending scan detail; Nanite candidate | 4K tiling concrete/brick, 2K trims, masks; lightmap UV if static bake variant used | Snap grid 10 cm, convex collision; intact/broken swappable pieces; HLOD groups must preserve doors |
| Interior kit (wall/floor/ceiling); 4 × 0.2 × 3.2 m wall | S; 2–20k; Nanite only when beneficial | 2–4K tileables, 512 px/m near review baseline, trim/unique damage masks | Simple collision, doorway clearance, occluder continuity; walls never only visual if server uses them for cover |
| Glass / door / shutter; 1–3 × 0.05–0.15 × 2.2–3.2 m | S; 0.2–8k; conventional translucency and constrained destruction | 1–2K roughness/dirt; pane thickness, transmission/refraction tested | Hinged/sliding transform plus server collision state; authored shards + cosmetic debris; open/close/breach |
| Relay cabinet; 0.9 × 0.45 × 1.8 m | S; 18–35k; static Nanite candidate | 2–4K metal atlas, service labels, emissive indicators | Simple convex body, hinge/socket; objective interaction; broken panel, sparks, power states |
| Street shelter / kiosk / bench; 1–6 m | S; 5–50k | Shared 2–4K civic trim and glass | Simple static collision; seats/cover Smart Object slots planned; constrained destruction |
| Furniture kit; 0.4–2.4 m | S; 2–30k each | 2K shared sets, hero objects 4K | Convex collision only for movable props; socketed interactions; low-cost break states |
| Road / curb / drain; 4–12 m modular | S; 1–15k base mesh; Nanite tessellation opt-in experiment | 4K asphalt/concrete tileables + RVT blend, drain/marking atlas | Continuous driving collision, no thin wheel snags; pothole/crater collision changes server-owned |
| Bridge / tower / crane; 15–160 m assemblies | S; 50k–1M authored assembly, instance subparts; Nanite candidate | Tileable 4K sets plus 8K only for proven hero coverage, UV trims | Segmented convex gameplay collision; HLOD skyline variant; large collapse is authored event, not unconstrained citywide Chaos |
| Tree; 3–22 m | 25–100k close geometric foliage or measured cards; Nanite foliage tested independently | 2–4K bark/leaf atlas, opacity strategy and WPO bounds | Trunk capsule/convex, no collision on leaves; seasonal/wind LOD; authoritative trunk break + cosmetic crown |
| Shrub / grass / reeds; 0.15–2.0 m | 0.2–12k clusters; instancing and overdraw budgets | Shared 1–2K atlas, terrain blending | Normally no gameplay collision; wind variation and density LOD; fire/flattening state only if gameplay needs it |
| Rocks / cliff kit; 0.3–40 m | S; 5–500k high-detail clusters; Nanite candidate | 4K tiling and macro variation, optional 8K source hero face | Coarse convex/height collision validated for traversal, HLOD; no high-frequency collision on scan noise |

### Rig and LOD contracts

**H human:** pelvis-root convention; twist bones for forearm/upper arm/thigh; clavicles, full articulated fingers, jaw, tongue, eyes and facial system compatible with the approved pipeline. Lock skeleton before mocap integration. Facial correctives handle lips/cheeks/eyelids and contact; expressions must not move teeth as soft tissue. MetaHuman is a candidate for licensed digital-human creation and facial capture, not an included character or a requirement that every NPC run maximum facial cost. Re-target approved body performances through IK Rig/IK Retargeter. Preserve hand contact, foot roll, shoulder posture, and believable center of mass.

**R robot:** rigid mechanical pivots match bearings; deform only boots/cables where required. Hit-zone IDs map to sensor, weapon, each limb, armor, power and cooling. Replicate functional disablement and stable damage stage, then drive sparks, limp and detachable local cosmetic fragments. Do not put damage exclusively on animated visual mesh traces if server bones are not evaluated consistently.

**V vehicle:** exact pivot and wheel radius, center-of-mass and collision proxy established with handling tests; export chassis/wheels as a reproducible skeletal rig. Working dashboard reads actual replicated/local derived vehicle telemetry. Mirrors use conditional render targets with resolution/update caps; no always-on full-cost planar captures. Production Chaos vehicles need a tested prediction/correction architecture; the native kinematic prototype does not establish realistic suspension or deformation.

LOD review uses screen size and projected feature size, not only fixed distances. Example human geometry reductions are 100%, 60%, 30%, 12%, 5%; at every step check skin seams, shadow silhouette, facial expression, accessories and animation transitions. Hair grooms transition to cards/meshes. Facial curves reduce before face geometry becomes too small to read. Animation Budget Allocator/Update Rate Optimization must respect attack/contact-critical updates. Replicated collision never silently changes because one client's visual LOD changed.

## Human motion and facial implementation recipe

`ABP_DF_Human` production graph:

1. Gather thread-safe movement speed/direction/acceleration, gait, stance, movement mode, aim yaw/pitch, injury and equipment tags. Networked gameplay state supplies stance/equipment; animation does not create authority.
2. Grounded state selects idle/start/locomotion/stop/pivot using stride and orientation warping where compatible; include acceleration-based starts and planted stop poses. Airborne selects jump start → fall loop → landing by predicted landing and authoritative movement mode. Water/ladder/prone need explicit states and server movement implementation before assets can enable them.
3. Cache lower-body pose. Layer weapon stance and aim offset through spine bones; additive breathing and recoil have per-view amplitude limits. Blend montage slots for reload/interaction/hit reactions using explicit interrupt rules.
4. Apply stride/foot placement with traces, pelvis offset and limits; disable foot lock during airborne and legitimate slides. Hand IK targets weapon sockets; steering/door/revive targets use motion-warping windows. Network only intent and montage state; avoid streaming every joint.
5. Apply bounded look-at, eye aim and facial curves after body motion. Eye focus has targets and dwell, small asynchronous saccades, variable blinking and blinks linked to gaze shifts; avoid random eye jitter. Speech curves drive lips/jaw/tongue with phoneme timing and emotion layers. Add breath/posture/shoulder shifts appropriate to exhaustion.
6. Solve cloth/hair at allowed LOD and quality, with teleport/reset rules. Animation notifies request local sound/FX, while server timers/state own hit and reload outcomes.

Required initial motion set: idle variants, walk/jog/sprint directional starts/stops, crouch, aim, fire, carbine and pistol reload, hit/death, one vehicle entry/exit, relay use, and one robot locomotion/attack/death. Planned set: crawl/prone, lean, vault/mantle/climb/slide, swim, ladder, cover, melee, carry/drag/revive. A state name in an enum is not implementation or mocap.

Capture: consent and release → shot/move list → calibrated stage/cameras → body/facial/audio timecode → solve → retarget → contact cleanup → facial/lip review → gameplay responsiveness pass → network montage/correction test. Source performances and uncompressed dialog are archived. Synthetic voice/face generation requires rights and explicit casting direction; no actor imitation. Hand-keying with professional cleanup is a valid fallback, but do not label uncaptured motion “motion captured.”

## Material graphs

Default project rendering remains a supported UE material path. **Substrate is opt-in**, pinned to the engine version, and requires a separate shader compile/performance pass before conversion. Do not globally enable it just to list a feature.

### `M_DF_EnvironmentLayered` (production asset to author)

Inputs: tileable base/normal/roughness, macro color/roughness, edge/dirt mask, cavity/AO, world-space wetness field, exposure mask, upward-normal mask, puddle basin mask, snow coverage. Use world-aligned mapping only where seams, normal orientation and cost are acceptable; trims and unique UVs remain preferable on manufactured surfaces.

Node contract: UV × tile scale → texture samples → neutral macro albedo multiplier clamped to material-calibrated range; blend normals with angle-corrected normal blending; roughness = calibrated dry roughness blended toward wet roughness by wetness × exposure; puddle mask = smoothstep(basin threshold, basin threshold + width, accumulated water) × upward-facing support; puddle normal uses two low-amplitude scrolling normal layers plus rain ripples; puddle layer changes optical response without claiming physically simulated water depth. Metallic follows exposed metal ID and becomes zero under opaque dirt/paint. AO modulates intended occlusion only, never a painted black albedo outline. Use decals sparingly and profile DBuffer overdraw.

`MPC_DF_Environment` native scalar contract: `RainIntensity`, `Wetness`, `SnowCoverage`, `WindSpeed`, `TimeOfDayHours`. Additional proposed scalars: `WindDirectionX/Y`, `TemperatureC`, `NightBlend`, `LightningLuminance`. A material collection is local presentation derived from replicated environment state. Character wetness/exposure are per-character parameters, not a global rain shortcut for indoor people. The collection and production material assets must still be authored and assigned to the World Director.

### `M_DF_VehiclePaint`

Base metal/paint substrate → colored base layer → calibrated flake normal/roughness at physically plausible scale → clearcoat layer with distinct coat normal. Add road grime low on the body using authored/world exposure masks, brake dust per wheel, damage revealing primer/metal through stable masks, and water beads only where visible. Windshield uses separate glass with interior/exterior dirt, raindrop flow texture, wiper swept mask and limited refraction. Do not render a high-frequency droplet simulation over every vehicle at all distances.

### `M_DF_Skin` / eyes / cloth

Skin uses calibrated base, micro/meso normals, roughness/oil masks and a tested subsurface profile; no baked specular highlights. Blood-flow/emotion variation stays subtle and actor-directed. Eyes have separate wet corneal surface, iris geometry/parallax as supported, tear line, correct refraction, and shadows/contact with lids. Teeth/tongue have their own roughness/subsurface response. Cloth uses weave normals at plausible scale, seam and wear masks, anisotropy only where justified, and wetness-dependent darkening/roughness/weight appearance; simulation handles silhouette and contact, not a shader fake.

8K is a source ceiling for selected hero faces, vehicles or large uniquely mapped materials. Determine runtime mip residency from expected screen coverage. Texture streaming, mipmaps and virtual textures are policies with measured memory limits. Runtime Virtual Textures blend terrain/road/foliage where supported; virtual texturing is not a universal solution for translucent or frequently changing surfaces. Height/displacement must have a supported renderer path and collision policy; a height map alone does not displace geometry.

## Niagara and destruction recipes

`NS_DF_Rain_Local`: camera-relative bounded volume, wind direction/velocity from environment, stretched streaks with exposure-aware visibility, collision only for the nearest limited set; reuse particles and cap spawn under scalability. Interior shelter mask/volumes suppress rain. Splash emitters spawn from nearby surface samples/event budgets, not one expensive trace for every drop. Separate windshield droplets, vehicle tire spray, gutter streams and pavement ripples have their own visibility conditions.

`NS_DF_Snow_Local`: multiple size/velocity bands, turbulence, shelter exclusion and wind; accumulation is material/field state, not billions of persistent particles. `NS_DF_Dust`: depth-faded sparse volumes, limited translucent layers. `NS_DF_Muzzle`: short flash, capped dynamic light, smoke, casing budget. `NS_DF_Impact`: physical-material lookup dispatches decal/sparks/dust/sound with per-frame event caps. `NS_DF_Explosion`: layered flash, debris, smoke and pressure impression; visibility/significance and scalability control simulation. GPU particles are cosmetic and never decide damage.

Author Geometry Collections for a small list of meaningful breakables. Server owns intact/cracked/breached/destroyed state and collision replacement. Replicate deterministic destruction trigger or documented Chaos replication only after testing; clients may simulate non-gameplay shards locally. Rejoin must reconstruct the correct blocking state. Bound fragment count/lifetime, remove tiny collision, pool decals, and cap smoke. Large collapses are designed sequences with stable gameplay collision states. Citywide unconstrained rigid-body demolition is not promised.

## Spatial audio architecture

The native prototype provides an audio playback API but bundles no sound recordings. Audio remains silent until supplied sound assets are connected. Production authored sources require original recordings or licensed libraries, editing, mixing, attenuation assets and listening tests.

Submix routing: `Master` → `WorldSFX` (`Weapons`, `Vehicles`, `Destruction`, `Ambience`, `Foley`) + `Dialogue` + `Music` + `UI` + `Voice`. Apply conservative bus headroom, peak protection and a measured loudness policy. Use a sidechain for dialogue intelligibility with subtle attack/release. Player sliders control meaningful groups; voice mute does not mute subtitles or essential mission indicators.

`MS_DF_Weapon` inputs: weapon profile, perspective, distance, indoor/outdoor send, suppression state, deterministic variation seed and fire event. Layers: mechanical bolt/action, muzzle blast, optional suppressor profile, supersonic crack only when projectile geometry warrants it, near reflection, environment tail, distant report; casing/reload are separate contact-synced events. Avoid summing every layer at equal level or adding a generic supersonic crack to subsonic rounds. Voice limiting prioritizes nearby threats and important distant events.

`MS_DF_Vehicle` inputs: smoothed RPM, throttle/load, gear, speed, slip, surface, damage and listener cabin state. Blend authored engine bands and intake/exhaust; transmission transients, suspension, tire surface loops, wind and interior resonances have independent envelopes. Exterior and cabin perspectives route through dedicated curves and acoustic sends. Replicate low-rate physical parameters, derive continuous sound locally, and handle dormancy/late join without replaying engine-start events.

Explosion layers: transient, controlled low-frequency energy, debris, glass, structure creak, exterior/interior tails and delayed distant report. Temporary hearing treatment is optional and bounded; provide reduced-tinnitus/disabled high-tone options and a separate loudness/night mode. Do not use painful peaks as realism. Hearing treatment affects presentation, not removal of all tactical accessibility information.

Use spatial attenuation and the selected UE/platform spatializer for mono world sources, with soundfield/multichannel beds for ambience where supported. Stereo, headphones and 7.1 are test configurations. Object-based output such as Dolby Atmos requires supported platform/plugin/licensing and end-to-end validation; it is not achieved by naming a submix “Atmos.”

Acoustics: rooms/portals define local reverberation, occlusion transmission, diffraction approximation and interior/exterior blend. Door state affects portal transmission. Trace-based occlusion is time-sliced and prioritized; one blocked ray should not silence sound. Wall material, thickness approximation and openings influence low-pass/attenuation. Distant gunfire/thunder propagation schedules arrival delay and appropriate tails. Middleware such as Wwise/FMOD or specialized acoustics is a procurement/integration decision; baseline engine attenuation/reverb remains usable. Native point-source attenuation alone is not environmental acoustic simulation.

Ambient director chooses district/time/weather/power-state layers: port cranes, hull knocks, HVAC, transformers, sparse civilian voices, traffic, sirens, gulls, rain drains and distant engagements. Sources are spatially anchored and virtualized when inaudible. Dialog plays from the speaker; radio uses a separately controlled bus with codec filtering and squad routing. Proximity voice requires a real online voice provider and moderation/accessibility controls, not a local audio component.

## UI, accessibility, and settings

Prototype HUD displays only state supported by the build. Production CommonUI screens: title/account gate, main menu, operation selection, party/squad, loadout, matchmaking, server browser if supported, profile, report player, and pause/settings. Inputs use Enhanced Input actions with keyboard/controller glyphs and remapping; avoid mixing raw-key logic into production widgets. A CommonUI dependency is not a completed CommonUI screen.

HUD hierarchy: objective and direction first, then health/ammunition, then interaction, squad condition and contextual feedback. Use legible typography and restrained motion; wetness, film grain and bloom cannot reduce text contrast. Team identity uses shapes/text as well as color. Subtitles support speaker name, size, background, directional context and non-speech captions. Separate controls for shake, head bob, motion blur, flashes, tinnitus, hold/toggle behavior, aim sensitivity, text/chat/voice and communication wheel. Photo mode cannot pause an authoritative multiplayer match or expose hidden enemies.

Graphics tiers LOW/MEDIUM/HIGH/ULTRA/CINEMATIC/EXTREME map to documented feature budgets, not six copies of the same setting. Competitive visibility/cover geometry remains equivalent. Expose resolution/upscale, HDR calibration, GI/reflections/shadows/textures/effects/vegetation/view distance/volumetrics, motion blur, grain, depth of field and chromatic aberration. TSR is the baseline candidate; DLSS/FSR/XeSS/frame generation require installed supported vendor plugins, runtime capability detection, licensing and QA. Cinematic/extreme and path-traced offline capture need clear performance expectations and are not competitive gameplay defaults.

Photo/replay camera specification: constrained live multiplayer free camera or offline/replay free camera; focal length, aperture, focus distance, exposure, composition guides and output scale; time/weather/posing edits only in authorized offline sessions/replays. High-resolution captures are tiled/tested offline to avoid memory exhaustion. Path tracing is a separate supported capture pipeline, not a replacement for real-time multiplayer rendering.

## Budget hypotheses and validation

These are initial targets for investigation on a declared high-end PC, **not measured results**. Target gameplay is 60 fps with a 16.67 ms frame budget at 2560×1440 output using calibrated upscaling, and a 30 Hz dedicated-server tick at 33.33 ms. Define GPU, CPU, VRAM, RAM, storage, engine revision and resolution before any result is publishable. Rendering overlap means CPU/GPU stage budgets are not simply additive.

| Measurement | Initial target to validate | Failure response |
|---|---|---|
| Client GPU | ≤ 15 ms p95 in active slice; reserve for traversal spikes | Reduce GI/reflection/shadow/volumetric/translucency cost with paired visual review |
| Client game thread | ≤ 8 ms p95 | Profile tick, AI presentation, UI, traces and allocations; batch/time-slice |
| Client render thread | ≤ 8 ms p95 | Material/mesh draw complexity, shadows, scene captures and instance policy |
| Server frame | ≤ 25 ms p95 and ≤ 33.33 ms p99 at the tested population | Reduce authoritative work density, optimize relevance/AI; do not advertise a larger population |
| Network per client | Initial 64 KiB/s downstream, 16 KiB/s upstream sustained hypotheses | Measure encoded bytes under battle/join/loss; tune state frequency/serialization/relevance |
| Client VRAM | ≤ 10 GiB resident hypothesis on a 12 GiB test card | Inspect textures, virtual shadow maps, render targets, grooms and transient allocations |
| Client RAM / server RAM | Set from measured slice baseline before expansion | Trace allocations; content residency and persistent actor limits |
| Traversal | No repeatable > 50 ms gameplay hitch after warm-up; report worst case | Streaming priorities, shader PSO coverage, IO, decompression and object creation |

Run Unreal Insights with CPU/frame/bookmark/memory/network channels as needed; GPU captures/ProfileGPU and stat commands corroborate bottlenecks. Keep shipping-like packaged builds, automated replay/route, cache conditions and settings consistent. Compare p50/p95/p99/worst frame, not only average FPS. Shader compilation and cold asset caches get separate reports from steady state.

Crowd and combat density tests must include renderer, server, replication and audio at once: a visually empty dedicated-server test cannot prove the client budget. Test 4→8→16→32→64 clients with separately specified AI/vehicles/effects, then stop expansion at the first failed gate. Simulated clients, editor windows and autonomous load bots are labeled distinctly; only realistic networked behavior establishes useful capacity.
