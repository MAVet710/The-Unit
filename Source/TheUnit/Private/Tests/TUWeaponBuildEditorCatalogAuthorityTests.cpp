#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponBuildEditor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponBuildEditorCatalogAuthorityTest,
    "TheUnit.Combat.WeaponBuildEditorCatalogAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponBuildEditorCatalogAuthorityTest::RunTest(const FString& Parameters)
{
    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_CatalogAuthority");
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = { ETUWeaponPartSlot::RailMount, ETUWeaponPartSlot::Optic };

    FWeaponPartDefinition CatalogRail;
    CatalogRail.PartId = TEXT("Part_Rail");
    CatalogRail.Slot = ETUWeaponPartSlot::RailMount;
    CatalogRail.RequiredInterfaceTags = { TEXT("Interface_Base") };
    CatalogRail.ProvidedInterfaceTags = { TEXT("Interface_Optic") };

    FWeaponPartDefinition CatalogOptic;
    CatalogOptic.PartId = TEXT("Part_Optic");
    CatalogOptic.Slot = ETUWeaponPartSlot::Optic;
    CatalogOptic.RequiredInterfaceTags = { TEXT("Interface_Optic") };

    const TArray<FWeaponPartDefinition> Catalog = { CatalogRail, CatalogOptic };

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    FString FailureReason;

    FWeaponPartDefinition ForgedRail = CatalogRail;
    ForgedRail.Slot = ETUWeaponPartSlot::Optic;
    ForgedRail.RequiredInterfaceTags.Reset();
    ForgedRail.ProvidedInterfaceTags = { TEXT("Interface_Forged") };

    TestTrue(TEXT("Known part ID installs using catalog authority"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, ForgedRail, Catalog, Build, FailureReason));
    TestEqual(TEXT("Caller cannot forge installed slot"),
        Build.InstalledParts[0].Slot, ETUWeaponPartSlot::RailMount);
    TestEqual(TEXT("Installed identity remains catalog identity"),
        Build.InstalledParts[0].PartId, CatalogRail.PartId);

    TestTrue(TEXT("Catalog-provided interface still permits dependent optic"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, CatalogOptic, Catalog, Build, FailureReason));

    const int32 StablePartCount = Build.InstalledParts.Num();
    FWeaponPartDefinition UnknownPart;
    UnknownPart.PartId = TEXT("Part_NotInCatalog");
    UnknownPart.Slot = ETUWeaponPartSlot::RailMount;
    UnknownPart.RequiredInterfaceTags.Reset();
    TestFalse(TEXT("Unknown install candidate is rejected"),
        FTUWeaponBuildEditor::TryInstallPart(Platform, UnknownPart, Catalog, Build, FailureReason));
    TestEqual(TEXT("Rejected unknown install leaves build unchanged"), Build.InstalledParts.Num(), StablePartCount);

    FWeaponPartDefinition ForgedReplacement = CatalogRail;
    ForgedReplacement.PartId = CatalogOptic.PartId;
    ForgedReplacement.Slot = ETUWeaponPartSlot::RailMount;
    TestTrue(TEXT("Known replacement ID also uses immutable catalog definition"),
        FTUWeaponBuildEditor::TryReplacePart(
            Platform, CatalogOptic.PartId, ForgedReplacement, Catalog, Build, FailureReason));
    const FWeaponInstalledPart* Replaced = Build.InstalledParts.FindByPredicate([&CatalogOptic](const FWeaponInstalledPart& Ref)
    {
        return Ref.PartId == CatalogOptic.PartId;
    });
    TestNotNull(TEXT("Replacement remains present"), Replaced);
    if (Replaced)
    {
        TestEqual(TEXT("Caller cannot forge replacement slot"), Replaced->Slot, ETUWeaponPartSlot::Optic);
    }

    FWeaponPartDefinition UnknownReplacement = UnknownPart;
    TestFalse(TEXT("Unknown replacement candidate is rejected"),
        FTUWeaponBuildEditor::TryReplacePart(
            Platform, CatalogOptic.PartId, UnknownReplacement, Catalog, Build, FailureReason));

    return true;
}

#endif
