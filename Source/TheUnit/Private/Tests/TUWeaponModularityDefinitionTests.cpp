#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TheUnitTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponModularityDefinitionsTest,
    "TheUnit.Combat.WeaponModularityDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponModularityDefinitionsTest::RunTest(const FString& Parameters)
{
    FWeaponPlatformDefinition Platform;
    TestFalse(TEXT("Platform has stable default identity"), Platform.PlatformId.IsNone());

    Platform.SupportedPartSlots = {
        ETUWeaponPartSlot::Barrel,
        ETUWeaponPartSlot::Optic,
        ETUWeaponPartSlot::FireControl
    };
    TestTrue(TEXT("Fire control is a first-class supported slot"),
        Platform.SupportedPartSlots.Contains(ETUWeaponPartSlot::FireControl));

    FWeaponPartDefinition Part;
    Part.PartId = TEXT("Part_Test_FireControl");
    Part.Slot = ETUWeaponPartSlot::FireControl;
    Part.RequiredInterfaceTags.Add(TEXT("Interface_Test"));
    Part.ProvidedInterfaceTags.Add(TEXT("Capability_Test"));

    TestFalse(TEXT("Part has stable identity"), Part.PartId.IsNone());
    TestTrue(TEXT("Part retains its slot"), Part.Slot == ETUWeaponPartSlot::FireControl);
    TestEqual(TEXT("Neutral recoil pitch multiplier"), Part.RecoilPitchMultiplier, 1.0f);
    TestEqual(TEXT("Neutral recoil yaw multiplier"), Part.RecoilYawMultiplier, 1.0f);
    TestEqual(TEXT("Neutral hip spread multiplier"), Part.HipSpreadMultiplier, 1.0f);
    TestEqual(TEXT("Neutral ADS spread multiplier"), Part.ADSSpreadMultiplier, 1.0f);
    TestEqual(TEXT("Neutral fire-rate multiplier"), Part.FireRateRPMMultiplier, 1.0f);

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    Build.SelectedAmmoId = TEXT("Ammo_Test");

    FWeaponInstalledPart InstalledPart;
    InstalledPart.Slot = Part.Slot;
    InstalledPart.PartId = Part.PartId;
    Build.InstalledParts.Add(InstalledPart);

    TestEqual(TEXT("Build retains platform identity"), Build.PlatformId, Platform.PlatformId);
    TestEqual(TEXT("Build retains installed part count"), Build.InstalledParts.Num(), 1);
    TestEqual(TEXT("Build retains installed part identity"), Build.InstalledParts[0].PartId, Part.PartId);
    TestTrue(TEXT("Build retains installed part slot"),
        Build.InstalledParts[0].Slot == ETUWeaponPartSlot::FireControl);
    TestEqual(TEXT("Build retains selected ammo identity"), Build.SelectedAmmoId, FName(TEXT("Ammo_Test")));

    return true;
}

#endif
