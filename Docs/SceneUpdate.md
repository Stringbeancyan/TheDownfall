# Breakwater presentation update

The old generator saved its sun at zero intensity and began play at 02:00.
The repository also required users to manually generate a map after compilation.
Both can produce an empty or almost black view even after a successful build.

## Open the update on Windows

1. Close Unreal Editor, then run `git pull origin main` in `C:\Users\nate\TheDownfall`.
2. Double-click **LaunchDownfall.bat** in that folder. It builds DownfallEditor
   with the installed UE 5.8 and opens the project only if the build succeeds.
   `UE_ROOT` or the first argument overrides the default engine folder.
3. On the first interactive launch, `Content/Python/init_unreal.py` runs the scene
   recipe after the editor opens. Wait for generation and shader compilation.
4. The resulting map is **Content > Downfall > Maps > L_Breakwater_District**.
   It should show a lit street before pressing Play. Use **Lit** viewport mode;
   enable **Game Settings** under the viewport exposure options if you previously
   forced a manual exposure. Press Play; F1 displays controls.

If auto-generation reports an error, open **Window > Output Log**. Save or discard
unsaved work, then run **Scripts/build_vertical_slice.py** using **Tools > Execute
Python Script**. Copy the first Python error if it fails. It deliberately refuses
to replace an existing completed district without `DOWNFALL_REBUILD_SLICE=1`.
`DOWNFALL_SKIP_AUTOBUILD=1` disables first-launch generation. Commandlets, server
and game processes never auto-generate content.

The original `L_Breakwater` remains untouched. The new scene is assembled in
`L_Breakwater_District_Build`, then saved to the final path after generation.
An interrupted first build does not save an empty map at the startup path. On
retry, only tagged actors in the staging world are replaced. Materials already
present are retained. These generated worlds are not substitutes for source
control backups. Complete generation before cooking or launching clients.

## Delivered source

- **Lighting:** saved daylight sun, movable skylight with atmosphere capture,
  bounded exposure, warm street lamps, illuminated interiors. Initial game time
  is 16:00; the replicated day/night clock continues to run.
- **District:** four enterable ground floors, upper façades, windows/mullions,
  roof machinery, rainwater pipes, benches, road drains, bollards, wayfinding,
  ribbed freight containers, pallets and distant building silhouettes.
- **Surfaces:** three original 512px packed TGA textures; world-space triplanar
  variation, roughness and fine bump. A shared dynamic material per surface
  follows replicated wetness and snow values. Snow is a shading approximation,
  not a simulation of accumulation; glass is opaque smoked glazing.
- **Models:** coloured component kits for the responder, Sentinel and utility car.
  Vehicle panels, glazing, mirrors, grille, rims and moving tires; body armour,
  equipment, joints and sensor. Collision remains the existing capsule/chassis.
  Walking and wheel motion are local cosmetic approximations. There is no skeletal
  motion capture, working suspension or detailed vehicle cabin in this update.
- **Rain:** up to 256 translucent instanced streaks around the first local camera,
  wind drift and eight roof traces per presentation tick. No server particles or
  per-particle replication. Roof-edge response is approximate; split-screen,
  snow particles and splashes remain pending.
- **HUD:** compact objective, compass, clock/weather, vitals and ammunition;
  contextual vehicle prompt, F1 help, cinematic bars and subtitles. Off-white and
  amber, without the persistent debug banner or full-width controls footer.

The scene is **PROTOTYPED** and its assembled geometry remains a **PLACEHOLDER**
for production assets. This is not a photorealistic asset delivery. It needs
authored/approved scans, human meshes, rigs, animation, weapon/vehicle assets and
recorded sound to reach the original visual target.

## Required local acceptance (not run in the cloud)

1. Clean native build in UE 5.8 / MSVC 14.44. Reopen the editor with the new module.
2. First-launch generation completes without Python or shader errors; report is
   `Saved/BreakwaterBuildReport.json`. Reopening preserves the saved map.
3. Inspect editor and Play views at 16:00, 02:00 and 06:00; do not compensate by
   setting the viewport to Unlit. In Details on World_WeatherAndTime, set Initial
   Hour before each Play test. Verify interiors and player silhouettes are readable.
4. Enter each doorway, capture the relay, reload, switch weapons, enter/exit and
   drive the car, fight the robot, die and respawn. Check collision matches the
   visible kit closely enough for this prototype; inspect wheel direction.
5. Watch rain become storm then fog/clear; verify wetness changes. Walk indoors
   and inspect roof-edge particle leakage. Check dedicated-server logs have no
   presentation errors and two clients share time/weather/objective state.
6. Inspect HUD at 1280x720, 1920x1080 and ultrawide; check subtitles and F1/Escape.
7. Cook with the generated directory included. Profile game/render/GPU time and
   draw calls before raising detail or player counts. No frame-rate claim yet.

Portable tests verify startup branching, failure reporting, unobstructed spawn
and doorway volumes, detail object budget, daylight recipe and texture data.
They mock Unreal and cannot establish engine API compatibility or visual quality.
