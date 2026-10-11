# DOWNFALL — production bible

**Title:** DOWNFALL. All named people, districts, organizations, vehicles, and story events below are original development fiction.

**Project state:** an initial Unreal Engine 5.6 C++ foundation and playable-blockout source, not a shipped game or a photorealistic asset package. An engine build, cooked dedicated server, hardware captures, animation performances, and production artwork remain acceptance work. See [FeatureStatus.md](FeatureStatus.md) for the evidence boundary.

## The game

An emergency crew crosses a coastal city whose autonomous civil-defense network has turned its evacuation infrastructure into a battlefield. Four players carry a stolen power-routing key through the Breakwater district while human factions and damaged municipal robots fight over a failing civic grid. The same spaces later support faction battles over transport, energy, and evacuation routes.

The repeated decision is **which part of the city to keep alive**. Teams scout a route, isolate a threat, secure a grid node, choose where its limited power goes, and move people or equipment through the resulting opening. Lighting, flood control, transit, surveillance, and robot reinforcements respond to grid state. Civilian consequences should be visible in streets and interiors; a numeric resource counter alone cannot carry this premise.

The initial operation is **Last Light**: reach a maintenance relay beside the port-side relay annex, survive a localized robot attack, and restore the clinic's flood pumps. A final vehicle escape becomes a later slice milestone. The first native blockout can validate movement, shots, damage, interactions, weather state, a robot, and a vehicle before this mission has production content.

There is no claim that one server can simulate the entire planned region at full fidelity. Operations select bounded districts. Large-war modes select larger contiguous battle areas with explicit player, actor, and bandwidth budgets. Persistent world services, if funded, store outcomes outside the frame-by-frame game simulation.

## Visual identity

Velmora is a fictional temperate Atlantic-facing port built on steep flood terraces. Its identity comes from pale salt-marked aggregate, dark oxidized steel, glazed transit tiles, reed-filled drainage channels, copper-colored municipal equipment, and old stone retaining walls under modern infrastructure. Civic typography and signs are original. Avoid reproducing a real city's skyline or another game's landmarks.

The defining scene is a 02:00 street after a transformer failure: cool overcast skylight, warm clinic windows, restrained cyan maintenance indicators, amber hazard lamps, and red navigation lights on cranes. Reflections are broken by aggregate, puddle depth, traffic grime, and wind; the street is not a uniform mirror. Silhouettes and human faces must remain readable without flooding every shadow with artificial fill.

Use measured, physically plausible material response. Exposed dielectrics usually have metallic=0; clean exposed metals have metallic=1; dirt masks transition between separate surface layers. Do not paint highlights into base color. Film references guide framing and exposure, not an exaggerated teal/orange grade. Grain, chromatic aberration, motion blur, lens distortion, and camera shake are individually disableable.

Robots are repurposed emergency machines: articulated stabilizer legs, shielded flood sensors, sealed joint boots, replaceable tool cassettes, and legible service numbers. They should look manufactured and repairable. Weapon-bearing variants visibly replace rescue tools. Exposed cooling and sensor assemblies create readable combat weak points without luminous arcade targets.

## Play modes and gates

| Mode | Intended structure | Ship/scale gate |
|---|---|---|
| Operations | Four-player authored missions; optional eight-player variant after encounter rework | First production target; four distinct clients plus a dedicated server must pass authority and join-in-progress tests |
| AI Siege | Four to sixteen players defend grid infrastructure from robot waves | Reuses tested robot, objectives, and wave director; count is measured, not assumed |
| Extraction | Small squads retrieve technology with bounded infiltration/extraction windows | Requires inventory persistence, economy abuse tests, reconnect policy, and service authentication |
| Frontline | Teams capture a moving sequence of objectives | Sixteen first; thirty-two and sixty-four only after server/network gates |
| Warzone | Combined-arms battles with infantry, ground vehicles, and later aircraft | Thirty-two/sixty-four are separate performance acceptance targets; no shared full-map simulation promise |
| Last Stand | Finite escalating defense with recoverable casualties | Requires squad revive and accessibility-tested intensity pacing |
| Experimental warfare | Counts greater than sixty-four | Research branch; no advertised capacity until repeatable packaged-build evidence exists |

Matchmaking, parties, friends, invites, crossplay, voice moderation, text moderation, reporting, account identity, and durable progression need a backend selection and platform agreements. The initial direct-connect session has none of those service guarantees. Do not represent a local name field or a listen-server address as account authentication.

## Region plan

The planned region is a **16 × 16 km design envelope**, not generated or populated production terrain. One Unreal unit is one centimeter; geographic source data, if introduced, must have a documented origin, coordinate transform, license, and precision budget. Keep the initial blockout near the world origin.

| Region | Footprint hypothesis | Play purpose and interior commitments |
|---|---|---|
| Breakwater | 1.6 × 1.4 km port edge | First 180 × 180 m route; clinic ground floor, relay room, garage, loading shelter, and service passage |
| Civic Rise | 2.0 × 2.0 km dense center | Municipal tower, hospital, station concourse, two apartment stair cores; access routes authored before facades |
| Glassbank | 2.5 × 1.5 km canal industry | Data center, turbine hall, robot depot, warehouse catwalks; acoustic and visibility compartmentalization |
| North Commons | 3.0 × 2.0 km suburbs | Homes, school refuge, retail street, emergency-service depots; reusable interior kits with distinctive hero rooms |
| Meridian Airfield | 3.0 × 2.5 km outer transport | Terminal zone, maintenance hangar, control annex; runways need vehicle and aerial sightline tests |
| Estuary Belt | 4.0 × 3.0 km water/coast | Freight port, bridge, flood tunnels, ferry terminal; boats arrive after networking and water physics prototypes |
| Redstone Uplands | 5.0 × 4.0 km rural/mountain envelope | Farms, forests, quarry, research bunker, military logistics base; low-density streaming workload |
| Subsurface network | Layered corridors under selected districts | Subway, sewers, shelters, maintenance tunnels; authored connections, separate streaming/occlusion volumes |

Footprints overlap and include buffer terrain. They are planning envelopes, not an additive area guarantee. Major highways and rail connect districts without making every distant tower enterable. Important buildings have a documented room graph, entrances, egress, collision, AI navigation, acoustic zones, power circuits, and streaming dependency. Decorative background buildings are explicitly marked as such in the content database.

### World Partition production setup

The generated first scene is a small native blockout and does not establish a working World Partition city. The production conversion is a separate editor task: create `L_Velmora_Main` as an Open World level, enable World Partition and One File Per Actor, migrate approved actors, and validate actor references before authoring subdistricts. Prototype a 128 m runtime grid and 384 m loading range; profile driving and helicopter approach before adopting those values. Use separate HLOD layers for small street clusters and skyline structures. Interiors use Data Layers and actor bounds carefully; do not unload a floor under an occupied player.

Server relevance is independent of client visual streaming. Keep authoritative objective and encounter state in persistent systems. A cell becoming invisible must not reset loot, duplicate enemies, erase damage, or teleport an occupied vehicle. World state persistence is a service/mission checkpoint concern, not an accidental property of level loading.

## Production phases with exit criteria

1. **Foundation and concept:** original setting, declared version, module boundaries, source control policy, source validation. Exit: Unreal Header Tool and client/server targets build on the selected licensed engine; no phase is complete merely because its document exists.
2. **Architecture:** server-authoritative core, observable state, input, health, weapon definitions, tests. Exit: two packaged clients see consistent state on a dedicated server; invalid client requests are rejected.
3. **Breakwater vertical slice:** four-player mission, two weapons, one ground vehicle, one robot, weather/time, bounded destruction, readable HUD, intro and spatial audio prototype. Exit: ten-minute complete loop with four clients; production visual/audio substitutions are tracked separately.
4. **System expansion:** injury/revive, inventory, armor/penetration, interaction framework, squad AI, mission graph, vehicle seats, GAS integration, accessibility, backend identity. Exit: subsystem acceptance suites and an agreed content workflow.
5. **Map/multiplayer expansion:** completed first district, sixteen-player Frontline, then thirty-two and sixty-four gates; World Partition/HLOD, relevance solution, distributed content ownership. Exit: cooked map streaming tests and measured load reports for each capacity.
6. **Optimization:** profile representative worst cases continuously, then feature freeze. Exit: frame/server/network budgets hold across the declared hardware matrix for a sustained match, including joins and traversal.
7. **QA/polish:** soak, exploit, desync, accessibility, localization, platform compliance, motion/audio/visual consistency. Exit: release criteria signed against a known build, known configuration, and repeatable evidence.

## Immediate production backlog

- Compile the checked-in native project using licensed Unreal Engine 5.6 and record exact engine changelist/compiler versions.
- Generate/open the blockout map, run a dedicated server and two clients, then complete the four-client test in [VerticalSlice.md](VerticalSlice.md).
- Replace one street bay, one room, one character, one weapon, and one vehicle with licensed or internally made art; evaluate a repeatable 02:00 beauty frame and an active firefight together.
- Validate the vehicle prototype's authority/correction behavior before adding Chaos vehicle simulation or multiple seats.
- Record an original Mira/Orin table read; establish consent, rights, pronunciation, and performance capture requirements before facial work.
- Select online identity/session and voice providers; implement authentication before any public playtest.
- Capture a baseline Unreal Insights, GPU, network, and memory trace before raising AI counts or map size.
