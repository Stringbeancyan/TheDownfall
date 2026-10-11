"""Generate the original Breakwater gameplay blockout inside Unreal Editor 5.6.

Build DownfallEditor first. Run from Tools > Execute Python Script or with:
  UnrealEditor Downfall.uproject -ExecutePythonScript=/absolute/path/to/this.py

This generates PLACEHOLDER primitive geometry, not photoreal production assets.
The first invocation creates and saves /Game/Downfall/Maps/L_Breakwater.
For an existing map, set DOWNFALL_REBUILD_SLICE=1 before launching the editor.
Explicit rebuild removes only actors tagged Downfall.Generated.Blockout;
untagged user actors and existing materials are retained. Save your work first.
No project content is downloaded and no external asset credentials are required.
"""
from __future__ import annotations

import json
import math
import os
from pathlib import Path

import unreal

MAP_PATH = "/Game/Downfall/Maps/L_Breakwater"
GENERATED_TAG = "Downfall.Generated.Blockout"
MATERIAL_ROOT = "/Game/Downfall/Generated/Materials"
REPORT_NAME = "BreakwaterBuildReport.json"
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assets = unreal.EditorAssetLibrary
spawned: list[unreal.Actor] = []


def tag(actor, *extra_tags):
    actor.set_editor_property("tags", [unreal.Name(GENERATED_TAG)] + [unreal.Name(t) for t in extra_tags])


def spawn(actor_class, label, position, rotation=(0, 0, 0), extra_tags=()):
    actor = actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(*position), unreal.Rotator(*rotation))
    if actor is None:
        raise RuntimeError(f"Unable to spawn {label}: {actor_class}")
    actor.set_actor_label(label)
    tag(actor, *extra_tags)
    spawned.append(actor)
    return actor


def constant_expression(material, value, x=0, y=0):
    expr = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, x, y)
    expr.set_editor_property("r", float(value))
    return expr


def make_material(name, color, roughness=0.7, metallic=0.0, emissive=0.0):
    path = f"{MATERIAL_ROOT}/{name}"
    if assets.does_asset_exist(path):
        material = assets.load_asset(path)
        if not isinstance(material, unreal.MaterialInterface):
            raise RuntimeError(f"Existing non-material occupies {path}")
        return material
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError(f"Unable to create material {path}")
    editor = unreal.MaterialEditingLibrary
    base = editor.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, 0)
    base.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    editor.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    editor.connect_material_property(constant_expression(material, roughness, -400, 130), "", unreal.MaterialProperty.MP_ROUGHNESS)
    editor.connect_material_property(constant_expression(material, metallic, -400, 230), "", unreal.MaterialProperty.MP_METALLIC)
    if emissive > 0:
        glow = editor.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, 330)
        glow.set_editor_property("constant", unreal.LinearColor(*(channel * emissive for channel in color), 1.0))
        editor.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    editor.recompile_material(material)
    if not assets.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save material {path}")
    return material


def box(label, position, dimensions, material, rotation=(0, 0, 0), collision=True):
    actor = spawn(unreal.StaticMeshActor, label, position, rotation)
    component = actor.static_mesh_component
    component.set_static_mesh(cube_mesh)
    component.set_material(0, material)
    component.set_mobility(unreal.ComponentMobility.STATIC)
    component.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    component.set_generate_overlap_events(False)
    actor.set_actor_scale3d(unreal.Vector(*(axis / 100.0 for axis in dimensions)))
    return actor


def text(label, message, position, rotation, size=35.0):
    actor = spawn(unreal.TextRenderActor, label, position, rotation)
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(message)
    component.set_world_size(size)
    component.set_text_render_color(unreal.Color(160, 222, 221, 255))
    return actor


def building(name, center, width, depth, height, front_sign, wall, floor, metal, neon):
    """Open ground floor, two real doors, roof, interior partitions and equipment.

    front_sign=-1 faces south; +1 faces north. Blockout has no window glass:
    openings remain visible and support weapon traces without false solidity.
    """
    x, y = center
    thickness = 32.0
    door_width = 260.0
    door_height = 330.0
    box(f"{name}_Floor", (x, y, 10), (width, depth, 20), floor)
    box(f"{name}_Roof", (x, y, height + 16), (width, depth, 32), wall)
    for side in (-1, 1):
        box(f"{name}_Side_{side}", (x + side * (width - thickness) / 2, y, height / 2), (thickness, depth, height), wall)
    for side in (-1, 1):
        face_y = y + side * (depth - thickness) / 2
        segment_width = (width - door_width) / 2
        for section in (-1, 1):
            box(f"{name}_Facade_{side}_{section}", (x + section * (door_width / 2 + segment_width / 2), face_y, height / 2), (segment_width, thickness, height), wall)
        box(f"{name}_DoorLintel_{side}", (x, face_y, door_height + (height - door_height) / 2), (door_width, thickness, height - door_height), wall)
        box(f"{name}_DoorThreshold_{side}", (x, face_y, 9), (door_width, 160, 18), metal)
    # A partition leaves passages at both ends; all wall collisions match geometry.
    box(f"{name}_InteriorPartition", (x + width * 0.19, y, 165), (24, depth * 0.42, 330), wall)
    for index in range(4):
        bx = x - width * 0.32 + index * 160
        box(f"{name}_InteriorWorktop_{index}", (bx, y + depth * 0.29, 95), (130, 80, 190), metal)
    front_y = y + front_sign * (depth / 2 + 4)
    box(f"{name}_Canopy", (x, front_y + front_sign * 140, 345), (620, 300, 24), metal)
    box(f"{name}_NeonBar", (x, front_y + front_sign * 286, 338), (520, 8, 12), neon, collision=False)
    # Text faces the street; it is art direction signage, not screen-space HUD.
    text(f"{name}_Sign", name.replace("_", " ").upper(), (x - 230, front_y + front_sign * 18, 425), (0, -90 if front_sign < 0 else 90, 0), 42)
    lamp = spawn(unreal.PointLight, f"{name}_InteriorLight", (x, y, 295))
    lamp.point_light_component.set_intensity(5000.0)
    lamp.point_light_component.set_attenuation_radius(2100.0)
    lamp.point_light_component.set_light_color(unreal.LinearColor(0.62, 0.76, 1.0, 1.0))
    lamp.point_light_component.set_cast_shadows(False)


def create_lighting():
    sun = spawn(unreal.DirectionalLight, "Breakwater_Sun", (0, 0, 12000), (-42, -28, 0), ("Downfall.Sun",))
    sun.directional_light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.directional_light_component.set_intensity(0.0)
    sun.directional_light_component.set_editor_property("atmosphere_sun_light", True)
    sun.directional_light_component.set_editor_property("atmosphere_sun_light_index", 0)
    moon = spawn(unreal.DirectionalLight, "Breakwater_Moon", (0, 0, 12500), (-35, 145, 0), ("Downfall.Moon",))
    moon.directional_light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    moon.directional_light_component.set_intensity(0.15)
    moon.directional_light_component.set_light_color(unreal.LinearColor(0.65, 0.76, 1.0, 1.0))
    moon.directional_light_component.set_editor_property("atmosphere_sun_light", True)
    moon.directional_light_component.set_editor_property("atmosphere_sun_light_index", 1)
    sky = spawn(unreal.SkyLight, "Breakwater_SkyLight", (0, 0, 1000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.light_component.set_intensity(0.4)
    spawn(unreal.SkyAtmosphere, "Breakwater_Atmosphere", (0, 0, 0))
    fog = spawn(unreal.ExponentialHeightFog, "Breakwater_AtmosphericFog", (0, 0, 120))
    fog.component.set_editor_property("fog_density", 0.025)
    fog.component.set_editor_property("volumetric_fog", True)
    for x in (-6100, -2900, 2900, 6100):
        for side in (-1, 1):
            y = side * 1120
            box(f"StreetlightPole_{x}_{side}", (x, y, 280), (18, 18, 560), materials["metal"])
            box(f"StreetlightArm_{x}_{side}", (x, y - side * 80, 545), (22, 170, 16), materials["metal"])
            lamp = spawn(unreal.PointLight, f"Streetlight_{x}_{side}", (x, y - side * 160, 515), extra_tags=("Downfall.NightLight",))
            lamp.point_light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            lamp.point_light_component.set_intensity(12000.0)
            lamp.point_light_component.set_attenuation_radius(1800.0)
            lamp.point_light_component.set_light_color(unreal.LinearColor(1.0, 0.69, 0.4, 1.0))
            lamp.point_light_component.set_cast_shadows(False)


def create_streets():
    box("District_Base_180m", (0, 0, -50), (18000, 18000, 100), materials["concrete"])
    box("Road_EastWest", (0, 0, 3), (18000, 1400, 6), materials["road"])
    box("Road_NorthSouth", (0, 0, 4), (1400, 18000, 8), materials["road"])
    for side in (-1, 1):
        box(f"Pavement_X_{side}", (0, side * 1100, 6), (18000, 720, 12), materials["concrete"])
        box(f"Pavement_Y_{side}", (side * 1100, 0, 7), (720, 18000, 14), materials["concrete"])
    for lane in range(-8, 9):
        if abs(lane) < 2:
            continue
        box(f"LaneMarking_X_{lane}", (lane * 950, 0, 8), (300, 12, 1), materials["marking"], collision=False)
        box(f"LaneMarking_Y_{lane}", (0, lane * 950, 9), (12, 300, 1), materials["marking"], collision=False)
    for side in (-1, 1):
        for stripe in range(9):
            box(f"Crosswalk_{side}_{stripe}", (-560 + stripe * 140, side * 1650, 10), (70, 280, 1), materials["marking"], collision=False)
    # A collision boundary makes this authored slice finite; full city streaming is planned.
    for side in (-1, 1):
        box(f"Boundary_X_{side}", (side * 8950, 0, 150), (100, 18000, 300), materials["concrete"])
        box(f"Boundary_Y_{side}", (0, side * 8950, 150), (18000, 100, 300), materials["concrete"])


def create_gameplay(native_classes):
    for index, start in enumerate(((-4600, -200, 120), (-4600, 200, 120), (-4900, -200, 120), (-4900, 200, 120))):
        spawn(unreal.PlayerStart, f"SquadSpawn_{index + 1}", start)
    spawn(native_classes["DownfallWorldDirector"], "World_WeatherAndTime", (0, 0, 0))
    spawn(native_classes["DownfallCaptureObjective"], "Objective_Relay", (1800, 2000, 0))
    spawn(native_classes["DownfallRobot"], "Sentinel_Prototype", (2700, 600, 110), (0, 180, 0))
    spawn(native_classes["DownfallVehicle"], "Vehicle_ServiceCar", (-1600, -420, 65))
    spawn(native_classes["DownfallCinematicDirector"], "Cinematic_LastLight", (0, 0, 0))
    for index, (x, y) in enumerate(((-3900, -3900), (3900, -3900), (-3900, 3900), (3900, 3900))):
        zone = spawn(native_classes["DownfallAcousticZone"], f"InteriorAcousticZone_{index + 1}", (x, y, 340))
        zone.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(2150, 1750, 330))
    for index, point in enumerate(((-2200, 580, 0), (-300, -580, 0), (1350, 590, 0), (2400, 1550, 0))):
        spawn(native_classes["DownfallDestructibleCover"], f"DestructibleCover_{index + 1}", point)
    text("DistrictTitle", "BREAKWATER / RELAY 03", (-3800, -1420, 185), (0, 90, 0), 65)
    # Readable geometry around the relay gives a defensible objective lane.
    for index, point in enumerate(((1000, 2600, 55), (2600, 2800, 55), (3300, 1200, 55))):
        box(f"UtilityCrate_{index + 1}", point, (140, 140, 110), materials["metal"])


def main():
    global cube_mesh, materials
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages() or unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError("Save or discard existing unsaved editor work before generating the slice.")
    existing = assets.does_asset_exist(MAP_PATH)
    if existing and os.environ.get("DOWNFALL_REBUILD_SLICE") != "1":
        raise RuntimeError(f"{MAP_PATH} exists. Set DOWNFALL_REBUILD_SLICE=1 to regenerate its tagged blockout actors explicitly.")
    cube_mesh = assets.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not isinstance(cube_mesh, unreal.StaticMesh):
        raise RuntimeError("Unreal's verified built-in Cube mesh was not found.")
    class_names = ("DownfallWorldDirector", "DownfallRobot", "DownfallCaptureObjective", "DownfallDestructibleCover", "DownfallVehicle", "DownfallCinematicDirector", "DownfallAcousticZone")
    native_classes = {name: unreal.load_class(None, f"/Script/Downfall.{name}") for name in class_names}
    missing = [name for name, cls in native_classes.items() if cls is None]
    if missing:
        raise RuntimeError(f"Build and load DownfallEditor before generating this map. Missing native classes: {missing}")
    assets.make_directory("/Game/Downfall/Maps")
    assets.make_directory(MATERIAL_ROOT)
    if existing:
        if not level_subsystem.load_level(MAP_PATH):
            raise RuntimeError(f"Unable to load {MAP_PATH}")
        for actor in actor_subsystem.get_all_level_actors():
            if unreal.Name(GENERATED_TAG) in actor.tags:
                if not actor_subsystem.destroy_actor(actor):
                    raise RuntimeError(f"Unable to regenerate tagged actor {actor.get_actor_label()}")
    elif not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError(f"Unable to create {MAP_PATH}")
    materials = {
        "concrete": make_material("M_Blockout_Concrete", (0.22, 0.235, 0.25), 0.82),
        "wall": make_material("M_Blockout_Masonry", (0.16, 0.18, 0.20), 0.87),
        "road": make_material("M_Blockout_Asphalt", (0.028, 0.033, 0.038), 0.36),
        "metal": make_material("M_Blockout_PaintedMetal", (0.10, 0.13, 0.15), 0.4, 0.55),
        "marking": make_material("M_Blockout_RoadPaint", (0.66, 0.64, 0.50), 0.6),
        "neon": make_material("M_Blockout_CyanSign", (0.03, 0.45, 0.55), 0.32, emissive=8.0),
    }
    create_streets()
    for name, center in (("Ferry_Exchange", (-3900, -3900)), ("Grid_Service", (3900, -3900)), ("Tide_Market", (-3900, 3900)), ("Relay_Annex", (3900, 3900))):
        building(name, center, 4400, 3600, 680, 1 if center[1] < 0 else -1, materials["wall"], materials["concrete"], materials["metal"], materials["neon"])
    create_lighting()
    create_gameplay(native_classes)
    if not level_subsystem.save_current_level():
        raise RuntimeError("Editor could not save the completed blockout level")
    report = {
        "status": "GENERATED_PLACEHOLDER_MAP",
        "map": MAP_PATH,
        "world_dimensions_m": [180, 180],
        "generated_actor_count": len(spawned),
        "player_starts": 4,
        "native_classes": list(native_classes),
        "generated_tag": GENERATED_TAG,
        "retained_user_actors": "All actors without generated_tag remain on explicit rebuild.",
        "validation_pending": ["Unreal automation suite", "two-client dedicated-server test", "visual art production", "performance profiling"],
        "limitations": ["Primitive blockout geometry and flat PBR materials", "No authored textures, dialogue recordings, rain particles, mocap or Sequencer asset", "Single sublevel, not a World Partition city"],
    }
    report_path = Path(unreal.Paths.project_saved_dir()) / REPORT_NAME
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    unreal.log(f"DOWNFALL: saved {MAP_PATH}; {len(spawned)} generated actors; report {report_path}")


if __name__ == "__main__":
    main()
