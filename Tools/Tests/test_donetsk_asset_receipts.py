import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
SCALE_RECEIPT = ROOT / "ExternalAssets" / "DonetskHighFidelity" / "scale_receipt.json"
VISUAL_RECEIPT = ROOT / "ExternalAssets" / "DonetskHighFidelity" / "visual_quality_receipt.json"


class DonetskManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_manifest_declares_baked_real_world_scale(self):
        self.assertEqual(self.manifest.get("version"), 4)
        assets = self.manifest.get("assets", [])
        self.assertEqual(len(assets), 10)
        for asset in assets:
            self.assertTrue(asset.get("baked_scale_expected"), asset["target_name"])
            self.assertGreater(float(asset.get("import_uniform_scale", 0.0)), 0.0)
            self.assertIn("material_override", asset)
            self.assertIn("nanite_required", asset)

    def test_importer_bakes_scale_in_fbx_import_options(self):
        importer = (
            ROOT / "Tools" / "Unreal" / "import_donetsk_high_fidelity_assets_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("Path(__file__).resolve().parents[2]", importer)
        self.assertIn("FbxImportUI", importer)
        self.assertIn("import_uniform_scale", importer)
        self.assertIn("reset_build_scale", importer)
        self.assertNotIn("OneDrive", importer)


class DonetskScaleReceiptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.receipt = json.loads(SCALE_RECEIPT.read_text(encoding="utf-8"))

    def test_scale_receipt_requires_all_assets_within_tolerance(self):
        self.assertEqual(self.receipt.get("schema"), "the-unit/donetsk-high-fidelity-scale/v3")
        self.assertEqual(self.receipt.get("status"), "success")
        self.assertEqual(self.receipt.get("required_count"), 10)
        self.assertEqual(self.receipt.get("failed_count"), 0)
        assets = self.receipt.get("assets", [])
        self.assertEqual(len(assets), 10)
        for asset in assets:
            name = asset["target_name"]
            self.assertTrue(asset.get("pass"), name)
            self.assertLessEqual(float(asset.get("relative_error", 1.0)), 0.035, name)
            build_scale = asset.get("build_scale3d", {})
            for axis in ("x", "y", "z"):
                self.assertLessEqual(abs(float(build_scale.get(axis, 0.0)) - 1.0), 0.001, name)

    def test_normalizer_is_verification_only(self):
        normalizer = (
            ROOT / "Tools" / "Unreal" / "normalize_donetsk_high_fidelity_scale_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("Path(__file__).resolve().parents[2]", normalizer)
        self.assertIn("Editor.AsyncStaticMeshCompilationFinishAll", normalizer)
        self.assertNotIn("set_lod_build_settings", normalizer)
        self.assertNotIn("apply_scale_factor", normalizer)
        self.assertNotIn("effective_dimensions_cm", normalizer)


class DonetskVisualQualityReceiptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.receipt = json.loads(VISUAL_RECEIPT.read_text(encoding="utf-8"))

    def test_visual_receipt_covers_exact_manifest_targets(self):
        expected = {asset["target_name"] for asset in self.manifest["assets"]}
        actual = {asset["target_name"] for asset in self.receipt.get("assets", [])}
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), 10)
        self.assertEqual(self.receipt.get("schema"), "the-unit/donetsk-visual-quality/v1")
        self.assertEqual(self.receipt.get("status"), "success")
        self.assertEqual(self.receipt.get("failed_count"), 0)

    def test_visual_assets_are_mechanically_production_ready(self):
        for asset in self.receipt.get("assets", []):
            name = asset["target_name"]
            self.assertTrue(asset.get("pass"), name)
            self.assertGreaterEqual(int(asset.get("material_slot_count", 0)), 1, name)
            self.assertFalse(asset.get("default_material_detected"), name)
            self.assertTrue(all(float(v) > 0.0 for v in asset["bounds_cm"].values()), name)
            self.assertLessEqual(float(asset.get("relative_scale_error", 1.0)), 0.035, name)
            self.assertEqual(len(asset.get("source_sha256", "")), 64, name)
            self.assertEqual(asset.get("manual_review"), "pending", name)
            build_scale = asset.get("build_scale3d", {})
            for axis in ("x", "y", "z"):
                self.assertLessEqual(abs(float(build_scale.get(axis, 0.0)) - 1.0), 0.001, name)
            if asset.get("nanite_required"):
                self.assertTrue(asset.get("nanite_enabled"), name)

    def test_sourcing_queue_has_explicit_state_for_every_target(self):
        queue = self.receipt.get("sourcing_queue", [])
        expected = {asset["target_name"] for asset in self.manifest["assets"]}
        self.assertEqual({item["target_name"] for item in queue}, expected)
        allowed = {"community_search", "community_selected", "generate_required", "approved"}
        for item in queue:
            self.assertIn(item.get("status"), allowed)

    def test_visual_validator_uses_direct_baked_bounds(self):
        validator = (
            ROOT / "Tools" / "Unreal" / "validate_donetsk_visual_assets_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("source_dimensions_cm", validator)
        self.assertNotIn("effective_dimensions_cm", validator)


if __name__ == "__main__":
    unittest.main()


class DonetskVisualQualityReceiptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.receipt = json.loads(VISUAL_RECEIPT.read_text(encoding="utf-8"))

    def test_visual_receipt_covers_exact_manifest_targets(self):
        expected = {asset["target_name"] for asset in self.manifest["assets"]}
        actual = {asset["target_name"] for asset in self.receipt.get("assets", [])}
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), 10)
        self.assertEqual(self.receipt.get("schema"), "the-unit/donetsk-visual-quality/v1")
        self.assertEqual(self.receipt.get("status"), "success")
        self.assertEqual(self.receipt.get("failed_count"), 0)

    def test_visual_assets_are_mechanically_production_ready(self):
        for asset in self.receipt.get("assets", []):
            name = asset["target_name"]
            self.assertTrue(asset.get("pass"), name)
            self.assertGreaterEqual(int(asset.get("material_slot_count", 0)), 1, name)
            self.assertFalse(asset.get("default_material_detected"), name)
            self.assertTrue(all(float(v) > 0.0 for v in asset["bounds_cm"].values()), name)
            self.assertLessEqual(float(asset.get("relative_scale_error", 1.0)), 0.035, name)
            self.assertEqual(len(asset.get("source_sha256", "")), 64, name)
            self.assertEqual(asset.get("manual_review"), "pending", name)
            build_scale = asset.get("build_scale3d", {})
            for axis in ("x", "y", "z"):
                self.assertLessEqual(abs(float(build_scale.get(axis, 0.0)) - 1.0), 0.001, name)
            if asset.get("nanite_required"):
                self.assertTrue(asset.get("nanite_enabled"), name)

    def test_sourcing_queue_has_explicit_state_for_every_target(self):
        queue = self.receipt.get("sourcing_queue", [])
        expected = {asset["target_name"] for asset in self.manifest["assets"]}
        self.assertEqual({item["target_name"] for item in queue}, expected)
        allowed = {"community_search", "community_selected", "generate_required", "approved"}
        for item in queue:
            self.assertIn(item.get("status"), allowed)

    def test_visual_validator_uses_direct_baked_bounds(self):
        validator = (
            ROOT / "Tools" / "Unreal" / "validate_donetsk_visual_assets_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("source_dimensions_cm", validator)
        self.assertNotIn("effective_dimensions_cm", validator)


if __name__ == "__main__":
    unittest.main()
