#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponDefinitionCatalog.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponDefinitionCatalogValidationTest,
    "TheUnit.Combat.WeaponDefinitionCatalogValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponDefinitionCatalogValidationTest::RunTest(const FString& Parameters)
{
    UTUWeaponDefinitionCatalog* Catalog = NewObject<UTUWeaponDefinitionCatalog>();
    if (!TestNotNull(TEXT("Catalog object"), Catalog))
    {
        return false;
    }

    FWeaponDefinition BaseWeapon;
    BaseWeapon.WeaponId = TEXT("WPN_Validation");

    FAmmoDefinition Ammo;
    Ammo.AmmoId = TEXT("Ammo_Validation");

    FFireControlModuleDefinition FireControl;
    FireControl.FireControlId = TEXT("FC_Validation");
    FireControl.SupportedFireModes = { ETUFireMode::SemiAuto, ETUFireMode::Burst };
    FireControl.BurstCount = 2;

    FTriggerDefinition Trigger;
    Trigger.TriggerId = TEXT("Trigger_Validation");
    Trigger.TriggerType = ETUTriggerType::TwoStage;

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_FC_Validation");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.FireControlDefinitionId = FireControl.FireControlId;

    FWeaponPartDefinition TriggerPart;
    TriggerPart.PartId = TEXT("Part_Trigger_Validation");
    TriggerPart.Slot = ETUWeaponPartSlot::Trigger;
    TriggerPart.TriggerDefinitionId = Trigger.TriggerId;

    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_Validation");
    Platform.BaseWeaponDefinitionId = BaseWeapon.WeaponId;
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = { ETUWeaponPartSlot::FireControl, ETUWeaponPartSlot::Trigger };
    Platform.CompatibleAmmoIds = { Ammo.AmmoId };

    Catalog->Platforms = { Platform };
    Catalog->BaseWeaponDefinitions = { BaseWeapon };
    Catalog->Parts = { FireControlPart, TriggerPart };
    Catalog->FireControlModules = { FireControl };
    Catalog->Triggers = { Trigger };
    Catalog->Ammunition = { Ammo };

    TArray<FString> Errors;
    TestTrue(TEXT("Valid catalog passes validation"), Catalog->ValidateCatalog(Errors));
    TestEqual(TEXT("Valid catalog reports no errors"), Errors.Num(), 0);

    FWeaponPartDefinition DuplicatePart = TriggerPart;
    Catalog->Parts.Add(DuplicatePart);
    TestFalse(TEXT("Duplicate part IDs are rejected"), Catalog->ValidateCatalog(Errors));
    TestTrue(TEXT("Duplicate part reports an error"), Errors.Num() > 0);
    Catalog->Parts.Pop();

    Catalog->Triggers.Reset();
    TestFalse(TEXT("Missing trigger behavior reference is rejected"), Catalog->ValidateCatalog(Errors));
    TestTrue(TEXT("Missing trigger behavior reports an error"), Errors.Num() > 0);
    Catalog->Triggers = { Trigger };

    Catalog->Platforms[0].CompatibleAmmoIds = { TEXT("Ammo_Missing") };
    TestFalse(TEXT("Platform missing ammo reference is rejected"), Catalog->ValidateCatalog(Errors));
    TestTrue(TEXT("Missing platform ammo reports an error"), Errors.Num() > 0);
    Catalog->Platforms[0].CompatibleAmmoIds = { Ammo.AmmoId };

    Catalog->FireControlModules[0].SupportedFireModes.Reset();
    TestFalse(TEXT("Fire control without modes is rejected"), Catalog->ValidateCatalog(Errors));
    TestTrue(TEXT("Invalid fire-control definition reports an error"), Errors.Num() > 0);
    Catalog->FireControlModules[0].SupportedFireModes = FireControl.SupportedFireModes;

    Catalog->Parts[0].FireControlDefinitionId = TEXT("FC_Missing");
    TestFalse(TEXT("Missing fire-control behavior reference is rejected"), Catalog->ValidateCatalog(Errors));
    Catalog->Parts[0].FireControlDefinitionId = FireControl.FireControlId;

    TestTrue(TEXT("Catalog returns to valid state after restoring mutations"), Catalog->ValidateCatalog(Errors));
    TestEqual(TEXT("Restored catalog has no errors"), Errors.Num(), 0);

    return true;
}

#endif
