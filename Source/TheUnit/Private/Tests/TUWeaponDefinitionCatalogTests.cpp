#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponBuildResolver.h"
#include "TUWeaponDefinitionCatalog.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponDefinitionCatalogTest,
    "TheUnit.Combat.WeaponDefinitionCatalog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponDefinitionCatalogTest::RunTest(const FString& Parameters)
{
    UTUWeaponDefinitionCatalog* Catalog = NewObject<UTUWeaponDefinitionCatalog>();
    if (!TestNotNull(TEXT("Catalog object"), Catalog))
    {
        return false;
    }

    FWeaponDefinition BaseWeapon;
    BaseWeapon.WeaponId = TEXT("WPN_Test");
    BaseWeapon.DisplayName = FText::FromString(TEXT("Test Weapon"));

    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_Test");
    Platform.BaseWeaponDefinitionId = BaseWeapon.WeaponId;
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = { ETUWeaponPartSlot::FireControl };
    Platform.CompatibleAmmoIds = { TEXT("Ammo_Test") };

    FAmmoDefinition Ammo;
    Ammo.AmmoId = TEXT("Ammo_Test");

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_FireControl");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.RequiredInterfaceTags = { TEXT("Interface_Base") };
    FireControlPart.FireControlDefinitionId = TEXT("FC_Test");

    FFireControlModuleDefinition FireControl;
    FireControl.FireControlId = TEXT("FC_Test");
    FireControl.TriggerProfileId = TEXT("Trigger.CatalogTest");
    FireControl.SupportedFireModes = { ETUFireMode::SemiAuto, ETUFireMode::Burst };
    FireControl.BurstCount = 2;

    Catalog->Platforms = { Platform };
    Catalog->BaseWeaponDefinitions = { BaseWeapon };
    Catalog->Ammunition = { Ammo };
    Catalog->Parts = { FireControlPart };
    Catalog->FireControlModules = { FireControl };

    FWeaponPlatformDefinition PlatformLookup;
    TestTrue(TEXT("Platform lookup succeeds"),
        Catalog->GetPlatformDefinition(Platform.PlatformId, PlatformLookup));
    TestEqual(TEXT("Platform resolves base weapon identity"),
        PlatformLookup.BaseWeaponDefinitionId, BaseWeapon.WeaponId);

    FWeaponPartDefinition PartLookup;
    TestTrue(TEXT("Part lookup succeeds"),
        Catalog->GetPartDefinition(FireControlPart.PartId, PartLookup));
    TestEqual(TEXT("Part lookup preserves slot"), PartLookup.Slot, ETUWeaponPartSlot::FireControl);

    FFireControlModuleDefinition FireControlLookup;
    TestTrue(TEXT("Fire-control lookup succeeds"),
        Catalog->GetFireControlDefinition(FireControl.FireControlId, FireControlLookup));
    TestEqual(TEXT("Fire-control lookup preserves trigger profile"),
        FireControlLookup.TriggerProfileId, FireControl.TriggerProfileId);

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    Build.SelectedAmmoId = Ammo.AmmoId;
    Build.InstalledParts = { { ETUWeaponPartSlot::FireControl, FireControlPart.PartId } };

    FTUResolvedWeaponBuild Resolved;
    FString FailureReason;
    TestTrue(TEXT("Catalog resolves valid build"),
        Catalog->ResolveWeaponBuild(Build, Resolved, FailureReason));
    TestTrue(TEXT("Resolved catalog build has no failure"), FailureReason.IsEmpty());
    TestTrue(TEXT("Resolved catalog build contains fire control"), Resolved.bHasFireControl);
    TestEqual(TEXT("Resolved trigger profile comes from catalog"),
        Resolved.FireControlDefinition.TriggerProfileId, FireControl.TriggerProfileId);

    FWeaponBuildState MissingAmmoBuild = Build;
    MissingAmmoBuild.SelectedAmmoId = TEXT("Ammo_Missing");
    TestFalse(TEXT("Catalog rejects missing ammunition definition"),
        Catalog->ResolveWeaponBuild(MissingAmmoBuild, Resolved, FailureReason));

    FWeaponBuildState MissingPlatformBuild = Build;
    MissingPlatformBuild.PlatformId = TEXT("Platform_Missing");
    TestFalse(TEXT("Catalog rejects missing platform"),
        Catalog->ResolveWeaponBuild(MissingPlatformBuild, Resolved, FailureReason));

    Catalog->BaseWeaponDefinitions.Reset();
    TestFalse(TEXT("Catalog rejects platform with missing base definition"),
        Catalog->ResolveWeaponBuild(Build, Resolved, FailureReason));

    return true;
}

#endif
