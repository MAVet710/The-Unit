import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "Tools" / "Reference" / "donetsk_decal_source_ledger.json"
RECEIPT = ROOT / "Saved" / "Automation" / "DonetskVisual" / "decal_receipt.json"

REQUIRED = {
    "MI_Donetsk_Decal_CrackedPlaster",
    "MI_Donetsk_Decal_ChippedPaint",
    "MI_Donetsk_Decal_ExposedSubstrate",
    "MI_Donetsk_Decal_Soot",
    "MI_Donetsk_Decal_RainStreaks",
    "MI_Donetsk_Decal_RustDrips",
    "MI_Donetsk_Decal_PatchedAsphalt",
    "MI_Donetsk_Decal_AsphaltCracks",
    "MI_Donetsk_Decal_TireWear",
    "MI_Donetsk_Decal_CurbGrime",
    "MI_Donetsk_Decal_UtilityMarking",
    "MI_Donetsk_Decal_FadedSignage",
    "MI_Donetsk_Decal_WaterStain",
}


class DonetskDecalGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
        cls.receipt = json.loads(RECEIPT.read_text(encoding="utf-8"))

    def test_decal_master_and_instance_count(self):
        self.assertEqual(self.receipt.get("schema"), "the-unit/donetsk-decal-quality/v1")
        self.assertEqual(self.receipt.get("status"), "success")
        self.assertEqual(self.receipt.get("failed_count"), 0)
        master = self.receipt["master"]
        self.assertEqual(master["asset_path"], "/Game/TheUnit/Donetsk/Decals/M_Donetsk_Decal_Master")
        self.assertEqual(master["material_domain"], "MD_DEFERRED_DECAL")
        self.assertTrue(master["dbuffer_compatible"])
        names = {item["name"] for item in self.receipt.get("instances", [])}
        self.assertTrue(REQUIRED.issubset(names))
        self.assertGreaterEqual(len(names), 12)

    def test_decal_inputs_and_size_limits(self):
        for item in self.receipt.get("instances", []):
            self.assertTrue(item["pass"], item["name"])
            self.assertTrue(item["texture_parameters"]["BaseColorTex"], item["name"])
            self.assertTrue(item["texture_parameters"]["NormalTex"], item["name"])
            self.assertTrue(item["texture_parameters"]["RoughnessTex"], item["name"])
            self.assertTrue(item["texture_parameters"]["OpacityTex"], item["name"])
            self.assertGreater(float(item["physical_size_cm"]["x"]), 0.0)
            self.assertGreater(float(item["physical_size_cm"]["y"]), 0.0)
            self.assertLessEqual(float(item["physical_size_cm"]["x"]), 800.0)
            self.assertLessEqual(float(item["physical_size_cm"]["y"]), 800.0)

    def test_source_ledger_covers_every_instance(self):
        entries = self.ledger.get("decals", [])
        by_name = {item["instance_name"]: item for item in entries}
        for name in REQUIRED:
            self.assertIn(name, by_name)
            item = by_name[name]
            self.assertEqual(item["license"], "Project-owned original")
            self.assertEqual(item["approval_state"], "approved")
            self.assertTrue(item["local_files"])
            self.assertIn(item["category"], {
                "facade_damage", "water_grime", "rust", "road_wear",
                "signage_remnants", "mission_impact",
            })

if __name__ == "__main__":
    unittest.main()
