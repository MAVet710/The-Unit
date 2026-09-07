#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponBuildResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponBuildResolverTest,
    "TheUnit.Combat.WeaponBuildResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponBuildResolverTest::RunTest(const FString& Parameters)
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
    Rail.PartId = TEXT("Part_Rail");
    Rail.Slot = ETUWeaponPartSlot::RailMount;
    Rail.RequiredInterfaceTags = { TEXT("Interface_Base") };
    Rail.ProvidedInterfaceTags = { TEXT("Interface_Optic") };
    Rail.RecoilPitchMultiplier = 0.9f;

    FWeaponPartDefinition Optic;
    Optic.PartId = TEXT("Part_Optic");
    Optic.Slot = ETUWeaponPartSlot::Optic;
    Optic.RequiredInterfaceTags = { TEXT("Interface_Optic") };
    Optic.ADSSpreadMultiplier = 0.5f;

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_FireControl");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.RequiredInterfaceTags = { TEXT("Interface_Base") };
    FireControlPart.FireControlDefinitionId = TEXT("FC_Test");

    FFireControlModuleDefinition FireControl;
    FireControl.FireControlId = TEXT("FC_Test");
    FireControl.TriggerProfileId = TEXT("Trigger.Test");
    FireControl.SupportedFireModes = { ETUFireMode::SemiAuto, ETUFireMode::Burst };
    FireControl.BurstCount = 2;

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    Build.SelectedAmmoId = TEXT("Ammo_Test");
    // Deliberately place the dependent optic before the rail provider; final build validation is order-independent.
    Build.InstalledParts = {
        { ETUWeaponPartSlot::Optic, Optic.PartId },
        { ETUWeaponPartSlot::RailMount, Rail.PartId },
        { ETUWeaponPartSlot::FireControl, FireControlPart.PartId }
    };

    FWeaponDefinition BaseDefinition;
    BaseDefinition.RecoilPitch = 2.0f;
    BaseDefinition.ADSSpread = 2.0f;

    FTUResolvedWeaponBuild Resolved;
    FString FailureReason;
    const bool bResolved = FTUWeaponBuildResolver::ResolveBuild(
        Platform,
        BaseDefinition,
        Build,
        { Rail, Optic, FireControlPart },
        { FireControl },
        Resolved,
        FailureReason);

    TestTrue(TEXT("Valid build resolves"), bResolved);
    TestTrue(TEXT("Valid build has no failure reason"), FailureReason.IsEmpty());
    TestEqual(TEXT("All installed definitions resolve"), Resolved.InstalledPartDefinitions.Num(), 3);
    TestEqual(TEXT("Derived recoil applies part modifier"), Resolved.DerivedWeaponDefinition.RecoilPitch, 1.8f);
    TestEqual(TEXT("Derived ADS spread applies part modifier"), Resolved.DerivedWeaponDefinition.ADSSpread, 1.0f);
    TestTrue(TEXT("Resolved build exposes fire control"), Resolved.bHasFireControl);
    TestEqual(TEXT("Resolved fire control identity"), Resolved.FireControlDefinition.FireControlId, FireControl.FireControlId);
    TestTrue(TEXT("Resolved fire control exposes burst"),
        Resolved.FireControlDefinition.SupportedFireModes.Contains(ETUFireMode::Burst));

    FWeaponBuildState MissingPartBuild = Build;
    MissingPartBuild.InstalledParts[0].PartId = TEXT("Part_Missing");
    TestFalse(TEXT("Missing catalog part is rejected"),
        FTUWeaponBuildResolver::ResolveBuild(
            Platform, BaseDefinition, MissingPartBuild,
            { Rail, Optic, FireControlPart }, { FireControl }, Resolved, FailureReason));

    FWeaponBuildState WrongAmmoBuild = Build;
    WrongAmmoBuild.SelectedAmmoId = TEXT("Ammo_Other");
    TestFalse(TEXT("Incompatible ammunition is rejected"),
        FTUWeaponBuildResolver::ResolveBuild(
            Platform, BaseDefinition, WrongAmmoBuild,
            { Rail, Optic, FireControlPart }, { FireControl }, Resolved, FailureReason));

    FWeaponPartDefinition BrokenOptic = Optic;
    BrokenOptic.RequiredInterfaceTags = { TEXT("Interface_Missing") };
    TestFalse(TEXT("Unsatisfied final-build interface is rejected"),
        FTUWeaponBuildResolver::ResolveBuild(
            Platform, BaseDefinition, Build,
            { Rail, BrokenOptic, FireControlPart }, { FireControl }, Resolved, FailureReason));

    return true;
}

#endif
