#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TUWeaponDefinitionCatalog.h"
#include "TUWeaponInstanceState.h"
#include "TU_WeaponBase.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponRuntimeInstanceHydrationTest,
    "TheUnit.Combat.WeaponRuntimeInstanceHydration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponRuntimeInstanceHydrationTest::RunTest(const FString& Parameters)
{
    UTUWeaponDefinitionCatalog* Catalog = NewObject<UTUWeaponDefinitionCatalog>();
    if (!TestNotNull(TEXT("Catalog object"), Catalog))
    {
        return false;
    }

    FWeaponDefinition BaseWeapon;
    BaseWeapon.WeaponId = TEXT("WPN_InstanceHydration");

    FAmmoDefinition Ammo;
    Ammo.AmmoId = TEXT("Ammo_InstanceHydration");
    Ammo.Damage = 41.0f;

    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_InstanceHydration");
    Platform.BaseWeaponDefinitionId = BaseWeapon.WeaponId;
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = { ETUWeaponPartSlot::FireControl, ETUWeaponPartSlot::Trigger };
    Platform.CompatibleAmmoIds = { Ammo.AmmoId };

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_FC_InstanceHydration");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.FireControlDefinitionId = TEXT("FC_InstanceHydration");

    FFireControlModuleDefinition FireControl;
    FireControl.FireControlId = FireControlPart.FireControlDefinitionId;
    FireControl.SupportedFireModes = { ETUFireMode::SemiAuto };

    FWeaponPartDefinition TriggerPart;
    TriggerPart.PartId = TEXT("Part_Trigger_InstanceHydration");
    TriggerPart.Slot = ETUWeaponPartSlot::Trigger;
    TriggerPart.TriggerDefinitionId = TEXT("Trigger_InstanceHydration");

    FTriggerDefinition Trigger;
    Trigger.TriggerId = TriggerPart.TriggerDefinitionId;
    Trigger.TriggerType = ETUTriggerType::Duty;

    Catalog->Platforms = { Platform };
    Catalog->BaseWeaponDefinitions = { BaseWeapon };
    Catalog->Ammunition = { Ammo };
    Catalog->Parts = { FireControlPart, TriggerPart };
    Catalog->FireControlModules = { FireControl };
    Catalog->Triggers = { Trigger };

    FWeaponInstanceState Instance;
    Instance.InstanceId = FGuid::NewGuid();
    Instance.Build.PlatformId = Platform.PlatformId;
    Instance.Build.SelectedAmmoId = Ammo.AmmoId;
    Instance.Build.InstalledParts = {
        { ETUWeaponPartSlot::FireControl, FireControlPart.PartId },
        { ETUWeaponPartSlot::Trigger, TriggerPart.PartId }
    };
    Instance.MagazineState.Capacity = 30;
    Instance.MagazineState.RoundsInMagazine = 7;
    Instance.MagazineState.bRoundChambered = true;
    Instance.AmmoReserve = 18;
    Instance.ConditionNormalized = 0.76f;

    const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        true, ERHIFeatureLevel::Num, &InitValues);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    ATU_WeaponBase* Weapon = World->SpawnActor<ATU_WeaponBase>();
    if (!TestNotNull(TEXT("Weapon actor"), Weapon))
    {
        World->DestroyWorld(false);
        return false;
    }

    FString FailureReason;
    TestTrue(TEXT("Persistent instance hydrates runtime weapon"),
        Weapon->ConfigureFromInstance(Catalog, Instance, FailureReason));
    TestTrue(TEXT("Hydrated runtime weapon tracks persistent identity"), Weapon->HasActiveInstance());
    TestEqual(TEXT("Persistent instance ID reaches runtime"), Weapon->GetActiveInstanceId(), Instance.InstanceId);
    TestEqual(TEXT("Condition reaches runtime"), Weapon->GetConditionNormalized(), Instance.ConditionNormalized);
    TestEqual(TEXT("Loaded state reaches runtime"), Weapon->GetCurrentAmmo(), 8);
    TestEqual(TEXT("Reserve state reaches runtime"), Weapon->GetReserveAmmo(), Instance.AmmoReserve);
    TestEqual(TEXT("Ammo definition reaches runtime"), Weapon->GetAmmoDefinition().AmmoId, Ammo.AmmoId);
    TestEqual(TEXT("Trigger definition reaches runtime"), Weapon->GetActiveTriggerId(), Trigger.TriggerId);

    Weapon->Fire();
    TestEqual(TEXT("Runtime mutation consumes one loaded round"), Weapon->GetCurrentAmmo(), 7);

    FWeaponInstanceState Exported;
    TestTrue(TEXT("Active runtime weapon exports a persistence snapshot"),
        Weapon->ExportActiveInstance(Exported, FailureReason));
    TestEqual(TEXT("Export preserves persistent identity"), Exported.InstanceId, Instance.InstanceId);
    TestEqual(TEXT("Export preserves build identity"), Exported.Build.PlatformId, Instance.Build.PlatformId);
    TestEqual(TEXT("Export captures mutated magazine rounds"), Exported.MagazineState.RoundsInMagazine, 6);
    TestTrue(TEXT("Export preserves chamber state"), Exported.MagazineState.bRoundChambered);
    TestEqual(TEXT("Export preserves reserve state"), Exported.AmmoReserve, Instance.AmmoReserve);
    TestEqual(TEXT("Export preserves condition"), Exported.ConditionNormalized, Instance.ConditionNormalized);

    const FGuid StableInstanceId = Weapon->GetActiveInstanceId();
    const int32 StableAmmo = Weapon->GetCurrentAmmo();
    FWeaponInstanceState Invalid = Instance;
    Invalid.ConditionNormalized = 2.0f;
    TestFalse(TEXT("Invalid instance rehydration is rejected"),
        Weapon->ConfigureFromInstance(Catalog, Invalid, FailureReason));
    TestEqual(TEXT("Rejected instance preserves active identity"), Weapon->GetActiveInstanceId(), StableInstanceId);
    TestEqual(TEXT("Rejected instance preserves runtime ammo"), Weapon->GetCurrentAmmo(), StableAmmo);

    TestTrue(TEXT("Anonymous catalog configuration still succeeds"),
        Weapon->ConfigureFromCatalog(Catalog, Instance.Build, FailureReason));
    TestFalse(TEXT("Anonymous configuration clears persistent identity"), Weapon->HasActiveInstance());
    TestFalse(TEXT("Anonymous configuration cannot export as a persistent item"),
        Weapon->ExportActiveInstance(Exported, FailureReason));

    World->DestroyWorld(false);
    return true;
}

#endif
