import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
USER_SETTINGS = ROOT / "Config" / "DefaultGameUserSettings.ini"
ENGINE_SETTINGS = ROOT / "Config" / "DefaultEngine.ini"
LAUNCHER = ROOT / "Scripts" / "RunDonetskUltraValidation.ps1"

EPIC_GROUPS = (
    "sg.ViewDistanceQuality=3",
    "sg.AntiAliasingQuality=3",
    "sg.ShadowQuality=3",
    "sg.TextureQuality=3",
    "sg.EffectsQuality=3",
    "sg.PostProcessQuality=3",
    "sg.GlobalIlluminationQuality=3",
    "sg.ReflectionQuality=3",
    "sg.ShadingQuality=3",
    "sg.FoliageQuality=3",
    "sg.LandscapeQuality=3",
)


class DonetskUltraProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.user = USER_SETTINGS.read_text(encoding="utf-8")
        cls.engine = ENGINE_SETTINGS.read_text(encoding="utf-8")
        cls.launcher = LAUNCHER.read_text(encoding="utf-8")

    def test_normal_user_settings_do_not_force_epic_or_cinematic(self):
        self.assertIn("sg.ViewDistanceQuality=2", self.user)
        self.assertIn("sg.GlobalIlluminationQuality=2", self.user)
        for group in EPIC_GROUPS:
            self.assertNotIn(group, self.user)
        self.assertNotRegex(self.user, r"sg\.[A-Za-z]+Quality=4")
        self.assertNotIn("r.Lumen.HardwareRayTracing", self.user)

    def test_ultra_launcher_requests_1440p_epic_tsr_lumen_vsm(self):
        self.assertIn("-ResX=2560", self.launcher)
        self.assertIn("-ResY=1440", self.launcher)
        for group in EPIC_GROUPS:
            self.assertIn(group, self.launcher)
        self.assertIn("r.AntiAliasingMethod=4", self.launcher)
        self.assertIn("r.DynamicGlobalIlluminationMethod=1", self.launcher)
        self.assertIn("r.ReflectionMethod=1", self.launcher)
        self.assertIn("r.Shadow.Virtual.Enable=1", self.launcher)
        self.assertIn("r.Nanite=1", self.launcher)
        self.assertIn("r.Lumen.HardwareRayTracing=1", self.launcher)
        self.assertIn("-noxgeshadercompile", self.launcher)

    def test_hardware_lumen_is_opt_in_but_project_support_exists(self):
        self.assertNotIn("r.Lumen.HardwareRayTracing", self.engine)
        self.assertIn("r.RayTracing=True", self.engine)
        self.assertIn("r.SkinCache.CompileShaders=True", self.engine)

    def test_launcher_targets_donetsk_and_writes_validation_log(self):
        self.assertIn("/Game/TheUnit/Maps/Donetsk", self.launcher)
        self.assertRegex(self.launcher, r"DonetskUltra.*\.log")
        self.assertIn("-game", self.launcher)


if __name__ == "__main__":
    unittest.main()
