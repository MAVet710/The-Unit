#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TUWeaponDefinitionCatalog.h"
#include "TU_WeaponBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponCatalogRuntimeConfigurationTest,
    "TheUnit.Combat.WeaponCatalogRuntimeConfiguration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponCatalogRuntimeConfigurationTest::RunTest(const FString& Parameters)
{
    UTUWeaponDefinitionCatalog* Catalog = NewObject<UTUWeaponDefinitionCatalog>();
    if (!TestNotNull(TEXT("Catalog object"), Catalog))
    {
        return false;
    }

    FWeaponDefinition BaseWeapon;
    BaseWeapon.WeaponId = TEXT("WPN_RuntimeTest");
    BaseWeapon.RecoilPitch = 2.0f;

    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_RuntimeTest");
    Platform.BaseWeaponDefinitionId = BaseWeapon.WeaponId;
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = { ETUWeaponPartSlot::FireControl };
    Platform.CompatibleAmmoIds = { TEXT("Ammo_RuntimeTest") };

    FAmmoDefinition Ammo;
    Ammo.AmmoId = TEXT("Ammo_RuntimeTest");
    Ammo.Damage = 52.0f;

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_RuntimeFireControl");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.RequiredInterfaceTags = { TEXT("Interface_Base") };
    FireControlPart.FireControlDefinitionId = TEXT("FC_RuntimeTest");

    FFireControlModuleDefinition FireControl;
    FireControl.FireControlId = FireControlPart.FireControlDefinitionId;
    FireControl.TriggerProfileId = TEXT("Trigger.RuntimeTest");
    FireControl.SupportedFireModes = { ETUFireMode::SemiAuto, ETUFireMode::Burst };
    FireControl.BurstCount = 2;

    Catalog->Platforms = { Platform };
    Catalog->BaseWeaponDefinitions = { BaseWeapon };
    Catalog->Ammunition = { Ammo };
    Catalog->Parts = { FireControlPart };
    Catalog->FireControlModules = { FireControl };

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    Build.SelectedAmmoId = Ammo.AmmoId;
    Build.InstalledParts = { { ETUWeaponPartSlot::FireControl, FireControlPart.PartId } };

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
    TestTrue(TEXT("Catalog configures runtime weapon"),
        Weapon->ConfigureFromCatalog(Catalog, Build, FailureReason));
    TestTrue(TEXT("Valid catalog configuration has no failure"), FailureReason.IsEmpty());
    TestEqual(TEXT("Selected ammo definition reaches runtime weapon"),
        Weapon->GetAmmoDefinition().AmmoId, Ammo.AmmoId);
    TestEqual(TEXT("Selected ammo gameplay data reaches runtime weapon"),
        Weapon->GetAmmoDefinition().Damage, Ammo.Damage);
    TestEqual(TEXT("Installed trigger profile reaches runtime weapon"),
        Weapon->GetActiveTriggerProfileId(), FireControl.TriggerProfileId);
    TestTrue(TEXT("Installed fire control exposes burst"),
        Weapon->GetAvailableFireModes().Contains(ETUFireMode::Burst));

    const FName StableAmmoId = Weapon->GetAmmoDefinition().AmmoId;
    const FName StableTriggerProfile = Weapon->GetActiveTriggerProfileId();
    FWeaponBuildState InvalidBuild = Build;
    InvalidBuild.SelectedAmmoId = TEXT("Ammo_Missing");
    TestFalse(TEXT("Missing catalog ammo rejects reconfiguration"),
        Weapon->ConfigureFromCatalog(Catalog, InvalidBuild, FailureReason));
    TestEqual(TEXT("Rejected configuration preserves prior ammo"),
        Weapon->GetAmmoDefinition().AmmoId, StableAmmoId);
    TestEqual(TEXT("Rejected configuration preserves prior fire control"),
        Weapon->GetActiveTriggerProfileId(), StableTriggerProfile);

    TestFalse(TEXT("Null catalog is rejected"),
        Weapon->ConfigureFromCatalog(nullptr, Build, FailureReason));

    World->DestroyWorld(false);
    return true;
}

#endif
