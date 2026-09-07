#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponBuildEditor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponBuildEditorTest,
    "TheUnit.Combat.WeaponBuildEditor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponBuildEditorTest::RunTest(const FString& Parameters)
{
    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_EditTest");
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = {
        ETUWeaponPartSlot::RailMount,
        ETUWeaponPartSlot::Optic,
        ETUWeaponPartSlot::FireControl
    };
    Platform.CompatibleAmmoIds = { TEXT("Ammo_A"), TEXT("Ammo_B") };

    FWeaponPartDefinition RailA;
    RailA.PartId = TEXT("Rail_A");
    RailA.Slot = ETUWeaponPartSlot::RailMount;
    RailA.RequiredInterfaceTags = { TEXT("Interface_Base") };
    RailA.ProvidedInterfaceTags = { TEXT("Interface_Optic") };

    FWeaponPartDefinition RailB = RailA;
    RailB.PartId = TEXT("Rail_B");

    FWeaponPartDefinition Optic;
    Optic.PartId = TEXT("Optic_A");
    Optic.Slot = ETUWeaponPartSlot::Optic;
    Optic.RequiredInterfaceTags = { TEXT("Interface_Optic") };

    FWeaponPartDefinition FireControlA;
    FireControlA.PartId = TEXT("FC_Part_A");
    FireControlA.Slot = ETUWeaponPartSlot::FireControl;
    FireControlA.RequiredInterfaceTags = { TEXT("Interface_Base") };

    FWeaponPartDefinition FireControlB = FireControlA;
    FireControlB.PartId = TEXT("FC_Part_B");

    const TArray<FWeaponPartDefinition> Catalog = {
        RailA, RailB, Optic, FireControlA, FireControlB
    };

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    FString FailureReason;

    TestFalse(TEXT("Dependent optic cannot install before its interface provider"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, Optic, Catalog, Build, FailureReason));
    TestTrue(TEXT("Rail provider installs"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, RailA, Catalog, Build, FailureReason));
    TestTrue(TEXT("Optic installs after provider"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, Optic, Catalog, Build, FailureReason));
    TestFalse(TEXT("Duplicate part identity is rejected"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, Optic, Catalog, Build, FailureReason));

    TestFalse(TEXT("Provider cannot be removed while another part depends on it"),
        FTUWeaponBuildEditor::TryRemovePart(Platform, RailA.PartId, Catalog, Build, FailureReason));
    TestTrue(TEXT("Provider can be atomically swapped for a compatible replacement"),
        FTUWeaponBuildEditor::TryReplacePart(Platform, RailA.PartId, RailB, Catalog, Build, FailureReason));
    TestTrue(TEXT("Replacement identity is present"),
        Build.InstalledParts.ContainsByPredicate([&RailB](const FWeaponInstalledPart& Ref)
        { return Ref.PartId == RailB.PartId; }));

    TestTrue(TEXT("First fire-control module installs"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, FireControlA, Catalog, Build, FailureReason));
    TestFalse(TEXT("Second simultaneous fire-control module is rejected"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, FireControlB, Catalog, Build, FailureReason));
    TestTrue(TEXT("Fire-control module can be atomically swapped"),
        FTUWeaponBuildEditor::TryReplacePart(
            Platform, FireControlA.PartId, FireControlB, Catalog, Build, FailureReason));

    TestTrue(TEXT("Compatible ammunition selection succeeds"),
        FTUWeaponBuildEditor::TrySelectAmmo(Platform, TEXT("Ammo_B"), Build, FailureReason));
    TestEqual(TEXT("Build stores selected ammunition identity"), Build.SelectedAmmoId, FName(TEXT("Ammo_B")));
    TestFalse(TEXT("Incompatible ammunition selection is rejected"),
        FTUWeaponBuildEditor::TrySelectAmmo(Platform, TEXT("Ammo_Invalid"), Build, FailureReason));
    TestEqual(TEXT("Rejected ammunition selection preserves prior selection"),
        Build.SelectedAmmoId, FName(TEXT("Ammo_B")));

    TestTrue(TEXT("Dependent optic can be removed"),
        FTUWeaponBuildEditor::TryRemovePart(Platform, Optic.PartId, Catalog, Build, FailureReason));
    TestTrue(TEXT("Provider can be removed after dependent part is gone"),
        FTUWeaponBuildEditor::TryRemovePart(Platform, RailB.PartId, Catalog, Build, FailureReason));

    return true;
}

#endif
