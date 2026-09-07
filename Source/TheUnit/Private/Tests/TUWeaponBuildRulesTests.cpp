#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponBuildRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponBuildRulesTest,
    "TheUnit.Combat.WeaponBuildRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponBuildRulesTest::RunTest(const FString& Parameters)
{
    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_Test");
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = {
        ETUWeaponPartSlot::RailMount,
        ETUWeaponPartSlot::Optic,
        ETUWeaponPartSlot::FireControl
    };
    Platform.CompatibleAmmoIds = { TEXT("Ammo_Test") };

    FWeaponPartDefinition Rail;
    Rail.PartId = TEXT("Part_Test_Rail");
    Rail.Slot = ETUWeaponPartSlot::RailMount;
    Rail.RequiredInterfaceTags = { TEXT("Interface_Base") };
    Rail.ProvidedInterfaceTags = { TEXT("Interface_OpticMount") };

    FString FailureReason;
    TArray<FWeaponPartDefinition> InstalledParts;
    TestTrue(TEXT("Base-compatible rail is accepted"),
        FTUWeaponBuildRules::IsPartCompatible(Platform, Rail, InstalledParts, &FailureReason));
    TestTrue(TEXT("Successful compatibility clears failure reason"), FailureReason.IsEmpty());

    FWeaponPartDefinition Optic;
    Optic.PartId = TEXT("Part_Test_Optic");
    Optic.Slot = ETUWeaponPartSlot::Optic;
    Optic.RequiredInterfaceTags = { TEXT("Interface_OpticMount") };

    TestFalse(TEXT("Dependent part fails before provider is installed"),
        FTUWeaponBuildRules::IsPartCompatible(Platform, Optic, InstalledParts));

    InstalledParts.Add(Rail);
    TestTrue(TEXT("Provider part enables dependent interface"),
        FTUWeaponBuildRules::IsPartCompatible(Platform, Optic, InstalledParts));

    FWeaponPartDefinition Unsupported;
    Unsupported.PartId = TEXT("Part_Test_Unsupported");
    Unsupported.Slot = ETUWeaponPartSlot::Underbarrel;
    TestFalse(TEXT("Unsupported slot is rejected"),
        FTUWeaponBuildRules::IsPartCompatible(Platform, Unsupported, InstalledParts));

    TestTrue(TEXT("Listed ammunition is compatible"),
        FTUWeaponBuildRules::IsAmmoCompatible(Platform, TEXT("Ammo_Test")));
    TestFalse(TEXT("Unlisted ammunition is incompatible"),
        FTUWeaponBuildRules::IsAmmoCompatible(Platform, TEXT("Ammo_Other")));

    FWeaponDefinition BaseDefinition;
    BaseDefinition.RecoilPitch = 2.0f;
    BaseDefinition.RecoilYaw = 1.0f;
    BaseDefinition.HipSpread = 4.0f;
    BaseDefinition.ADSSpread = 2.0f;
    BaseDefinition.FireRateRPM = 600.0f;

    Rail.RecoilPitchMultiplier = 0.9f;
    Rail.RecoilYawMultiplier = 1.1f;
    Rail.HipSpreadMultiplier = 0.8f;
    Rail.ADSSpreadMultiplier = 0.75f;
    Rail.FireRateRPMMultiplier = 1.0f;

    Optic.RecoilPitchMultiplier = 1.0f;
    Optic.RecoilYawMultiplier = 1.0f;
    Optic.HipSpreadMultiplier = 1.0f;
    Optic.ADSSpreadMultiplier = 0.5f;
    Optic.FireRateRPMMultiplier = 1.0f;

    const FWeaponDefinition Derived = FTUWeaponBuildRules::DeriveWeaponDefinition(
        BaseDefinition, { Rail, Optic });

    TestEqual(TEXT("Derived recoil pitch"), Derived.RecoilPitch, 1.8f);
    TestEqual(TEXT("Derived recoil yaw"), Derived.RecoilYaw, 1.1f);
    TestEqual(TEXT("Derived hip spread"), Derived.HipSpread, 3.2f);
    TestEqual(TEXT("Derived ADS spread"), Derived.ADSSpread, 0.75f);
    TestEqual(TEXT("Derived fire rate"), Derived.FireRateRPM, 600.0f);

    return true;
}

#endif
