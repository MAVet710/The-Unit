import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
SCALE_RECEIPT = ROOT / "ExternalAssets" / "DonetskHighFidelity" / "scale_receipt.json"


class DonetskScaleReceiptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.receipt = json.loads(SCALE_RECEIPT.read_text(encoding="utf-8"))

    def test_scale_receipt_requires_all_assets_within_tolerance(self):
        self.assertEqual(self.manifest.get("version"), 3)
        self.assertEqual(len(self.manifest.get("assets", [])), 10)
        self.assertEqual(self.receipt.get("status"), "success")
        self.assertEqual(self.receipt.get("required_count"), 10)
        self.assertEqual(self.receipt.get("failed_count"), 0)
        assets = self.receipt.get("assets", [])
        self.assertEqual(len(assets), 10)
        by_name = {asset["target_name"]: asset for asset in assets}
        self.assertEqual(len(by_name), 10)
        for asset in assets:
            self.assertTrue(asset.get("pass"), asset.get("target_name"))
            self.assertLessEqual(float(asset.get("relative_error", 1.0)), 0.035)

        target = by_name["SM_Donetsk_Khrush_5F_12"]
        measured = float(target.get("measured_after_cm", 0.0))
        self.assertGreaterEqual(measured, 1447.5)
        self.assertLessEqual(measured, 1552.5)

    def test_normalizer_uses_project_relative_root(self):
        normalizer = (
            ROOT / "Tools" / "Unreal" / "normalize_donetsk_high_fidelity_scale_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("Path(__file__).resolve().parents[2]", normalizer)
        self.assertNotIn("OneDrive\\Documents\\GitHub\\The-Unit", normalizer)

    def test_normalizer_waits_for_static_mesh_compilation(self):
        normalizer = (
            ROOT / "Tools" / "Unreal" / "normalize_donetsk_high_fidelity_scale_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("Editor.AsyncStaticMeshCompilationFinishAll", normalizer)
        self.assertIn("Editor.AsyncStaticMeshCompilation 0", normalizer)

    def test_normalizer_uses_one_correction_per_editor_launch(self):
        normalizer = (
            ROOT / "Tools" / "Unreal" / "normalize_donetsk_high_fidelity_scale_editor.py"
        ).read_text(encoding="utf-8")
        self.assertNotIn("while iterations < MAX_ITERATIONS", normalizer)
        self.assertIn('"needs_verification"', normalizer)

    def test_scale_receipt_is_idempotent(self):
        self.assertEqual(
            self.receipt.get("schema"),
            "the-unit/donetsk-high-fidelity-scale/v2",
        )
        for asset in self.receipt.get("assets", []):
            self.assertIn("iterations", asset)
            self.assertIn("build_scale_before", asset)
            self.assertIn("build_scale_after", asset)
            factor = float(asset.get("idempotence_factor", 0.0))
            self.assertLessEqual(abs(factor - 1.0), 0.035, asset["target_name"])

    def test_idempotence_threshold_matches_scale_tolerance(self):
        normalizer = (
            ROOT / "Tools" / "Unreal" / "normalize_donetsk_high_fidelity_scale_editor.py"
        ).read_text(encoding="utf-8")
        self.assertIn("IDEMPOTENCE_TOLERANCE = TOLERANCE", normalizer)


if __name__ == "__main__":
    unittest.main()
