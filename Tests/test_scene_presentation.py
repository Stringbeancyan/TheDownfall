"""Portable scene/boot contracts. These do not validate Unreal's rendering APIs."""
import importlib.util
import configparser
from pathlib import Path
import runpy
import struct
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import MagicMock, patch

ROOT = Path(__file__).resolve().parents[1]


def load_script(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FirstLaunchTests(unittest.TestCase):
    def test_editor_game_server_cook_and_launcher_select_the_generated_map(self):
        fake = MagicMock()
        with patch.dict(sys.modules, {"unreal": fake}):
            recipe = load_script("slice_map_contract", ROOT / "Scripts/build_vertical_slice.py")
        launcher = load_script("launcher_map_contract", ROOT / "Scripts/run_multiplayer.py")
        self.assertEqual(launcher.MAP, recipe.MAP_PATH)
        engine = configparser.ConfigParser(strict=False)
        engine.read(ROOT / "Config/DefaultEngine.ini")
        maps = engine["/Script/EngineSettings.GameMapsSettings"]
        for key in ("EditorStartupMap", "GameDefaultMap", "ServerDefaultMap"):
            self.assertEqual(maps[key], recipe.MAP_PATH)
        game = configparser.ConfigParser(strict=False)
        game.read(ROOT / "Config/DefaultGame.ini")
        self.assertIn(recipe.MAP_PATH, game["/Script/UnrealEd.ProjectPackagingSettings"]["+MapsToCook"])

    def boot(self, existing=False, command_line="", failure=False):
        unreal = MagicMock()
        unreal.EditorAssetLibrary.does_asset_exist.return_value = existing
        unreal.SystemLibrary.get_command_line.return_value = command_line
        unreal.Paths.project_dir.return_value = str(ROOT)
        namespace = {"__name__": "test_bootstrap"}
        generator = MagicMock(side_effect=RuntimeError("test failure") if failure else None)
        with patch.dict(sys.modules, {"unreal": unreal}), patch.dict("os.environ", {"DOWNFALL_SKIP_AUTOBUILD": "0"}), patch.object(runpy, "run_path", generator):
            exec(compile((ROOT / "Content/Python/init_unreal.py").read_text(), "init_unreal.py", "exec"), namespace)
            if unreal.register_slate_post_tick_callback.called:
                callback = unreal.register_slate_post_tick_callback.call_args.args[0]
                callback(.5)
                generator.assert_not_called()
                callback(1.6)
        return unreal, generator

    def test_missing_map_builds_once_after_editor_tick_and_unregisters(self):
        unreal, generator = self.boot()
        generator.assert_called_once_with(str(ROOT / "Scripts/build_vertical_slice.py"), run_name="__main__")
        unreal.unregister_slate_post_tick_callback.assert_called_once()

    def test_existing_user_map_is_never_regenerated(self):
        unreal, generator = self.boot(existing=True)
        unreal.register_slate_post_tick_callback.assert_not_called()
        generator.assert_not_called()

    def test_cooks_servers_clients_and_explicit_generation_do_not_autobuild(self):
        for arguments in ("-run=Cook", "-server", "-game", "-ExecutePythonScript=foo.py", "-unattended"):
            with self.subTest(arguments=arguments):
                unreal, generator = self.boot(command_line=arguments)
                generator.assert_not_called()
                unreal.register_slate_post_tick_callback.assert_not_called()

    def test_failure_is_visible_and_does_not_leave_a_repeating_callback(self):
        unreal, generator = self.boot(failure=True)
        generator.assert_called_once()
        unreal.unregister_slate_post_tick_callback.assert_called_once()
        unreal.EditorDialog.show_message.assert_called_once()
        unreal.log_error.assert_called_once()


class DistrictGeometryTests(unittest.TestCase):
    def setUp(self):
        self.boxes = []
        self.actors = []
        unreal = MagicMock()
        with patch.dict(sys.modules, {"unreal": unreal}):
            module = load_script("district_details", ROOT / "Scripts/district_details.py")

        def box(name, position, dimensions, material, **kwargs):
            self.boxes.append((name, position, dimensions, kwargs.get("collision", True)))

        def spawn(cls, name, position):
            self.actors.append((name, position))
            return SimpleNamespace(static_mesh_component=MagicMock(), set_actor_scale3d=MagicMock())

        materials = {key: key for key in ("wall", "concrete", "metal", "glass", "rubber", "wood", "neon")}
        module.dress_district(box, spawn, MagicMock(), materials)

    def test_kit_stays_within_small_district_object_budget(self):
        self.assertGreater(len(self.boxes), 500)
        self.assertLess(len(self.boxes) + len(self.actors), 1300)
        names = [item[0] for item in self.boxes] + [item[0] for item in self.actors]
        self.assertEqual(len(names), len(set(names)))
        for name, _, dimensions, _ in self.boxes:
            self.assertTrue(all(value > 0 for value in dimensions), name)

    def test_details_do_not_block_ground_floor_doorways_or_squad_spawns(self):
        clear_points = [(-4600,-200,120), (-4600,200,120), (-4900,-200,120), (-4900,200,120)]
        for x, y in ((-3900,-3900), (3900,-3900), (-3900,3900), (3900,3900)):
            clear_points.extend([(x, y - 1800, 120), (x, y + 1800, 120)])
        for name, center, dimensions, collision in self.boxes:
            if not collision:
                continue
            for point in clear_points:
                # A 72cm-wide, 184cm-tall player fits through the clearance.
                overlaps = all(abs(point[axis] - center[axis]) < dimensions[axis] / 2 + (36,36,92)[axis]
                               for axis in range(3))
                self.assertFalse(overlaps, f"{name} obstructs {point}")

    def test_editor_preview_has_real_sunlight_and_bounded_exposure(self):
        fake = MagicMock()
        with patch.dict(sys.modules, {"unreal": fake}):
            module = load_script("slice_recipe", ROOT / "Scripts/build_vertical_slice.py")
        spawned = {}

        def spawn(cls, label, *args, **kwargs):
            spawned[label] = MagicMock()
            return spawned[label]

        module.spawn = spawn
        module.box = MagicMock()
        module.materials = {"metal": object()}
        module.create_lighting()
        intensity = spawned["Breakwater_Sun"].directional_light_component.set_intensity.call_args.args[0]
        self.assertGreater(intensity, 10000)
        self.assertIn("Breakwater_Atmosphere", spawned)
        self.assertIn("Breakwater_SkyLight", spawned)
        self.assertIn("District_Exposure", spawned)


class SurfaceSourceTests(unittest.TestCase):
    def test_committed_sources_are_reproducible_nonconstant_linear_data(self):
        module = load_script("surface_sources", ROOT / "Scripts/make_surface_textures.py")
        for kind in ("Concrete", "Asphalt", "Metal"):
            with self.subTest(kind=kind):
                data = (ROOT / f"ArtSource/Surfaces/T_{kind}_Surface.tga").read_bytes()
                header = struct.unpack("<BBBHHBHHHHBB", data[:18])
                self.assertEqual(header[-4:], (512,512,24,32))
                self.assertEqual(len(data), 18 + 512 * 512 * 3)
                self.assertEqual(data[18:], module.make_pixels(kind))
                for channel in range(3):
                    self.assertGreater(len(set(data[18+channel::3])), 10)


if __name__ == "__main__":
    unittest.main()
