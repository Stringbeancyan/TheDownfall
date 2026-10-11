"""Meaningful authoring-contract tests; no Unreal runtime is exercised here."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("validate_assets", ROOT / "Scripts/validate_assets.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class AssetManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads((ROOT / "Content/Downfall/AssetManifest.json").read_text(encoding="utf-8"))

    def with_first(self, **changes):
        manifest = copy.deepcopy(self.manifest)
        manifest["assets"][0].update(changes)
        return manifest

    def test_production_specifications_satisfy_contract(self):
        self.assertEqual(module.validate_manifest(self.manifest), [])

    def test_duplicate_identity_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["assets"].append(copy.deepcopy(manifest["assets"][0]))
        errors = module.validate_manifest(manifest)
        self.assertTrue(any("duplicate id" in message for message in errors))
        self.assertTrue(any("duplicate content_path" in message for message in errors))

    def test_nan_and_nonphysical_dimensions_rejected(self):
        for dimensions in ([1, 2, float("nan")], [-1, 2, 3], [1, 0, 3], [True, 1, 1], [1, 2]):
            with self.subTest(dimensions=dimensions):
                self.assertTrue(any("dimensions_m" in message for message in module.validate_manifest(self.with_first(dimensions_m=dimensions))))

    def test_out_of_budget_and_non_power_of_two_textures_rejected(self):
        for resolution in (16384, 3000, 0, -4096, True):
            with self.subTest(resolution=resolution):
                manifest = copy.deepcopy(self.manifest)
                manifest["assets"][0]["texture_sets"][0]["source_resolution"] = resolution
                self.assertTrue(any("source_resolution" in message for message in module.validate_manifest(manifest)))

    def test_incomplete_pbr_channels_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["assets"][0]["texture_sets"][0]["channels"] = ["base_color"]
        self.assertTrue(any("PBR channels" in message for message in module.validate_manifest(manifest)))

    def test_non_decreasing_lod_budgets_rejected(self):
        for ratios in ([1, 0.5, 0.75], [1, 1], [0.5, 0.2], [1, 0], [1, "half"]):
            with self.subTest(ratios=ratios):
                manifest = self.with_first(lod={"triangle_ratios": ratios, "strategy": "Authoring test"})
                self.assertTrue(any("lod.triangle_ratios" in message for message in module.validate_manifest(manifest)))

    def test_skeletal_assets_must_name_skeleton(self):
        manifest = self.with_first(rigging={"type": "skeletal"})
        self.assertTrue(any("require skeleton" in message for message in module.validate_manifest(manifest)))

    def test_content_path_cannot_escape_project_namespace(self):
        for path in ("/Engine/Hero", "/Game/Downfall/../Other/Hero", "C:\\Mesh.fbx", None):
            with self.subTest(path=path):
                self.assertTrue(any("content_path" in message for message in module.validate_manifest(self.with_first(content_path=path))))

    def test_malformed_nested_data_reports_errors_instead_of_crashing(self):
        for field in ("topology", "nanite", "rigging", "uv", "texture_sets", "lod", "materials", "collision"):
            with self.subTest(field=field):
                self.assertTrue(module.validate_manifest(self.with_first(**{field: None})))
        self.assertTrue(module.validate_manifest(None))
        self.assertTrue(module.validate_manifest({"assets": [None]}))


if __name__ == "__main__":
    unittest.main()
