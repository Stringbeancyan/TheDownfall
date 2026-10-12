"""Original Breakwater architecture kit in centimetres. Called inside Unreal.

All pieces use physical scale and simple collision. Detail never seals the
ground-floor doorways, the four squad spawns, or the main combat lanes.
"""
import unreal


def dress_district(box, spawn, text, materials):
    wall, concrete, metal = (materials[k] for k in ("wall", "concrete", "metal"))
    glass, rubber, wood = (materials[k] for k in ("glass", "rubber", "wood"))
    glow = materials["neon"]
    cylinder = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")

    def pipe(name, pos, radius, height, material=metal):
        actor = spawn(unreal.StaticMeshActor, name, pos)
        component = actor.static_mesh_component
        component.set_static_mesh(cylinder)
        component.set_material(0, material)
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector(radius / 50, radius / 50, height / 100))
        return actor

    for index, (x, y) in enumerate(((-3900, -3900), (3900, -3900), (-3900, 3900), (3900, 3900))):
        name = f"Block_{index}"
        side = 1 if y < 0 else -1
        front = y + side * 1800
        # Occupied ground floor remains enterable. Upper storeys are exterior kit.
        upper_height = (1800, 2600, 1400, 2100)[index]
        box(f"{name}_UpperMass", (x, y, 700 + upper_height / 2), (4360, 3560, upper_height), wall)
        for level in range(3):
            z = 890 + level * 430
            box(f"{name}_FloorBand_{level}", (x, front + side * 8, z - 110), (4400, 54, 30), concrete)
            for bay in range(8):
                wx = x - 1850 + bay * 530
                box(f"{name}_WindowFrame_{level}_{bay}", (wx, front + side * 12, z + 90), (356, 22, 252), metal, collision=False)
                box(f"{name}_Window_{level}_{bay}", (wx, front + side * 27, z + 90), (320, 8, 220), glass, collision=False)
                box(f"{name}_Mullion_{level}_{bay}", (wx, front + side * 36, z + 90), (8, 14, 220), metal, collision=False)
                box(f"{name}_Sill_{level}_{bay}", (wx, front + side * 45, z - 28), (370, 80, 16), concrete, collision=False)
        for offset in (-2100, -1080, 1080, 2100):
            box(f"{name}_Pilaster_{offset}", (x + offset, front, 340), (64, 88, 680), concrete)
        for side_x in (-1, 1):
            pipe(f"{name}_Drain_{side_x}", (x + side_x * 2150, front + side * 45, 800), 9, 1600)
        roof_z = 700 + upper_height
        for machine in range(3):
            mx = x - 1200 + machine * 1000
            box(f"{name}_HVAC_{machine}", (mx, y, roof_z + 110), (420, 240, 220), metal)
            for vent in range(5):
                box(f"{name}_Vent_{machine}_{vent}", (mx - 160 + vent * 80, y, roof_z + 228), (34, 200, 8), rubber, collision=False)
        # Interior fixtures: tables with separated legs, shelf bays and wall lights.
        for desk in range(3):
            dx, dy = x - 1400 + desk * 680, y - side * 600
            box(f"{name}_Desk_{desk}", (dx, dy, 85), (240, 95, 8), wood)
            for lx in (-100, 100):
                for ly in (-32, 32):
                    box(f"{name}_DeskLeg_{desk}_{lx}_{ly}", (dx + lx, dy + ly, 40), (7, 7, 80), metal)
            box(f"{name}_Console_{desk}", (dx, dy, 105), (52, 14, 32), rubber)
            box(f"{name}_ConsoleScreen_{desk}", (dx, dy + side * 8, 105), (44, 1, 24), glass, collision=False)
        for shelf in range(3):
            box(f"{name}_Shelf_{shelf}", (x - 1900, y, 70 + shelf * 70), (90, 800, 10), metal)
        for sy in (-390, 390):
            box(f"{name}_RackLeg_{sy}", (x - 1900, y + sy, 110), (85, 8, 220), metal)
        for offset in (-1100, 1100):
            box(f"{name}_InteriorPractical_{offset}", (x + offset, y, 590), (200, 24, 12), glow, collision=False)

    # Road edges and storm drains give the pavement a readable thickness.
    for sign in (-1, 1):
        for i in range(-8, 9):
            if abs(i) < 2:
                continue
            box(f"Curb_{sign}_{i}", (i * 900, sign * 735, 11), (884, 25, 22), concrete)
        for x in (-5300, -1800, 1900, 5200):
            box(f"DrainFrame_{x}_{sign}", (x, sign * 696, 9), (100, 55, 6), metal, collision=False)
            for slat in range(6):
                box(f"DrainSlot_{x}_{sign}_{slat}", (x - 40 + slat * 16, sign * 696, 13), (7, 43, 2), rubber, collision=False)
        for x in (-7200, -5500, -900, 900, 5400, 7100):
            pipe(f"Bollard_{x}_{sign}", (x, sign * 820, 48), 8, 96)
            pipe(f"BollardCap_{x}_{sign}", (x, sign * 820, 89), 8.5, 9, glow)
        for x in (-5700, -2400, 2200, 5600):
            # Five separate timber slats, back and steel feet.
            for slat in range(5):
                box(f"BenchSeat_{x}_{sign}_{slat}", (x, sign * 1260 + slat * 10, 48), (185, 8, 6), wood)
            for z in (73, 87, 101):
                box(f"BenchBack_{x}_{sign}_{z}", (x, sign * 1315, z), (185, 7, 10), wood)
            for foot in (-65, 65):
                box(f"BenchFrame_{x}_{sign}_{foot}", (x + foot, sign * 1280, 24), (8, 60, 48), metal)
        pipe(f"WayfindingPost_{sign}", (sign * 1450, -1250, 160), 5, 320)
        box(f"WayfindingBoard_{sign}", (sign * 1450, -1250, 295), (190, 12, 65), metal)
        text(f"WayfindingText_{sign}", "RELAY 03  >" if sign > 0 else "FERRY TERMINAL", (sign * 1450 - 83, -1258, 298), (0, -90, 0), 17)

    # Freight court: ribbed containers, doors, corner castings, actual pallets.
    for container, (x, y) in enumerate(((-6900, -6100), (-6900, 4200), (6600, 6100))):
        box(f"Freight_{container}_Body", (x, y, 130), (610, 244, 260), metal)
        for rib in range(18):
            for side in (-1, 1):
                box(f"Freight_{container}_Rib_{rib}_{side}", (x - 285 + rib * 33, y + side * 125, 130), (8, 8, 248), metal, collision=False)
        for side in (-1, 1):
            box(f"Freight_{container}_Door_{side}", (x + 308, y + side * 59, 132), (10, 112, 240), wall)
            pipe(f"Freight_{container}_Lock_{side}", (x + 319, y + side * 55, 132), 3, 217)
        for pallet in range(3):
            px, py = x + 350, y + 400 + pallet * 180
            for slat in range(6):
                box(f"Pallet_{container}_{pallet}_{slat}", (px - 50 + slat * 20, py, 18), (15, 100, 5), wood)
            for runner in (-40, 40):
                box(f"PalletFoot_{container}_{pallet}_{runner}", (px, py + runner, 9), (120, 12, 14), wood)

    # Distant silhouette outside the playable boundary; no inaccessible interiors
    # are presented as playable buildings.
    for index in range(16):
        x = -15000 + (index % 8) * 4200
        y = -15500 if index < 8 else 15500
        height = 2400 + ((index * 137) % 7) * 600
        box(f"Skyline_{index}", (x, y, height / 2), (2400, 2200, height), wall, collision=False)
        for storey in range(4):
            box(f"SkylineBand_{index}_{storey}", (x, y + (1110 if y < 0 else -1110), 900 + storey * 450), (2260, 12, 160), glass, collision=False)
