import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "Tools" / "Reference" / "donetsk_surface_source_ledger.json"
RECEIPT = ROOT / "Saved" / "Automation" / "DonetskVisual" / "material_receipt.json"

MASTERS = {
    "/Game/TheUnit/Donetsk/Materials/M_Donetsk_Surface_Master",
    "/Game/TheUnit/Donetsk/Materials/M_Donetsk_Glass_Master",
    "/Game/TheUnit/Donetsk/Materials/M_Donetsk_Foliage_Master",
}
INSTANCES = {
    "MI_Donetsk_Asphalt",
    "MI_Donetsk_Paving",
    "MI_Donetsk_AgedConcrete",
    "MI_Donetsk_CurbConcrete",
    "MI_Donetsk_Plaster",
    "MI_Donetsk_PanelWarm",
    "MI_Donetsk_PanelCool",
    "MI_Donetsk_Brick",
    "MI_Donetsk_GalvanizedMetal",
    "MI_Donetsk_RustedSteel",
    "MI_Donetsk_Wood",
    "MI_Donetsk_Soil",
    "MI_Donetsk_WetGround",
}


class DonetskMaterialGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
        cls.receipt = json.loads(RECEIPT.read_text(encoding="utf-8"))

    def test_receipt_has_required_masters_and_instances(self):
        self.assertEqual(self.receipt.get("schema"), "the-unit/donetsk-material-quality/v1")
        self.assertEqual(self.receipt.get("status"), "success")
        self.assertEqual(self.receipt.get("failed_count"), 0)
        masters = {item["asset_path"] for item in self.receipt.get("masters", [])}
        self.assertTrue(MASTERS.issubset(masters))
        instances = {item["name"] for item in self.receipt.get("instances", [])}
        self.assertTrue(INSTANCES.issubset(instances))

    def test_master_surface_parameters_are_present(self):
        surface = next(
            item for item in self.receipt["masters"]
            if item["asset_path"].endswith("M_Donetsk_Surface_Master")
        )
        required = {
            "BaseColorTex",
            "NormalTex",
            "RoughnessTex",
            "MetallicTex",
            "UVScale",
            "MacroTint",
            "MacroRoughness",
            "DetailNormalStrength",
            "Wetness",
            "Variation",
            "WorldAlignedFallback",
        }
        self.assertTrue(required.issubset(set(surface["parameters"])))
        self.assertTrue(surface["dbuffer_compatible"])

    def test_texture_encoding_and_metallic_rules(self):
        for texture in self.receipt.get("textures", []):
            kind = texture["kind"]
            if kind == "Normal":
                self.assertFalse(texture["srgb"], texture["asset_path"])
                self.assertEqual(texture["compression"], "TC_NORMALMAP")
            elif kind in {"Roughness", "Metallic"}:
                self.assertFalse(texture["srgb"], texture["asset_path"])
        for instance in self.receipt.get("instances", []):
            if instance.get("physical_surface") not in {"galvanized_metal", "rusted_steel", "vehicle_paint"}:
                self.assertLessEqual(float(instance.get("metallic_max", 0.0)), 0.05, instance["name"])

    def test_source_ledger_is_complete_and_approved(self):
        entries = self.ledger.get("surfaces", [])
        by_name = {item["name"]: item for item in entries}
        for name in INSTANCES:
            surface_name = name.removeprefix("MI_Donetsk_")
            self.assertIn(surface_name, by_name)
            item = by_name[surface_name]
            self.assertIn(item.get("license"), {"Project-owned original", "CC0"})
            self.assertTrue(item.get("local_files"))
            self.assertEqual(item.get("approval_state"), "approved")
            self.assertGreaterEqual(int(item.get("resolution", 0)), 1024)


if __name__ == "__main__":
    unittest.main()
