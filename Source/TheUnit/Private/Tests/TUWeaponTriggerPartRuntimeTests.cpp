#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TUWeaponBuildEditor.h"
#include "TUWeaponDefinitionCatalog.h"
#include "TU_WeaponBase.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponTriggerPartRuntimeTest,
    "TheUnit.Combat.WeaponTriggerPartRuntime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponTriggerPartRuntimeTest::RunTest(const FString& Parameters)
{
    UTUWeaponDefinitionCatalog* Catalog = NewObject<UTUWeaponDefinitionCatalog>();
    if (!TestNotNull(TEXT("Catalog object"), Catalog))
    {
        return false;
    }

    FWeaponDefinition BaseWeapon;
    BaseWeapon.WeaponId = TEXT("WPN_TriggerTest");

    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_TriggerTest");
    Platform.BaseWeaponDefinitionId = BaseWeapon.WeaponId;
    Platform.InterfaceTags = { TEXT("Interface_Base") };
    Platform.SupportedPartSlots = { ETUWeaponPartSlot::FireControl, ETUWeaponPartSlot::Trigger };
    Platform.CompatibleAmmoIds = { TEXT("Ammo_TriggerTest") };

    FAmmoDefinition Ammo;
    Ammo.AmmoId = TEXT("Ammo_TriggerTest");

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_FireControl");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.RequiredInterfaceTags = { TEXT("Interface_Base") };
    FireControlPart.FireControlDefinitionId = TEXT("FC_TriggerTest");

    FFireControlModuleDefinition FireControl;
    FireControl.FireControlId = FireControlPart.FireControlDefinitionId;
    FireControl.SupportedFireModes = { ETUFireMode::SemiAuto, ETUFireMode::Burst };
    FireControl.BurstCount = 2;
    FireControl.TriggerProfileId = TEXT("Legacy.Trigger");
    FireControl.TriggerResponseMultiplier = 9.0f;
    FireControl.ResetResponseMultiplier = 9.0f;

    FWeaponPartDefinition TriggerPartA;
    TriggerPartA.PartId = TEXT("Part_Trigger_A");
    TriggerPartA.Slot = ETUWeaponPartSlot::Trigger;
    TriggerPartA.RequiredInterfaceTags = { TEXT("Interface_Base") };
    TriggerPartA.TriggerDefinitionId = TEXT("Trigger_A");

    FTriggerDefinition TriggerA;
    TriggerA.TriggerId = TriggerPartA.TriggerDefinitionId;
    TriggerA.DisplayName = FText::FromString(TEXT("Two Stage Test Trigger"));
    TriggerA.TriggerType = ETUTriggerType::TwoStage;
    TriggerA.TriggerResponseMultiplier = 0.72f;
    TriggerA.ResetResponseMultiplier = 1.25f;
    TriggerA.SemiAutoResetDelaySeconds = 0.03f;
    TriggerA.bRequiresReleaseBetweenSemiShots = false;

    FWeaponPartDefinition TriggerPartB = TriggerPartA;
    TriggerPartB.PartId = TEXT("Part_Trigger_B");
    TriggerPartB.TriggerDefinitionId = TEXT("Trigger_B");

    FTriggerDefinition TriggerB = TriggerA;
    TriggerB.TriggerId = TriggerPartB.TriggerDefinitionId;
    TriggerB.TriggerType = ETUTriggerType::Match;
    TriggerB.TriggerResponseMultiplier = 0.55f;
    TriggerB.ResetResponseMultiplier = 1.4f;

    Catalog->Platforms = { Platform };
    Catalog->BaseWeaponDefinitions = { BaseWeapon };
    Catalog->Ammunition = { Ammo };
    Catalog->Parts = { FireControlPart, TriggerPartA, TriggerPartB };
    Catalog->FireControlModules = { FireControl };
    Catalog->Triggers = { TriggerA, TriggerB };

    FWeaponBuildState Build;
    Build.PlatformId = Platform.PlatformId;
    Build.SelectedAmmoId = Ammo.AmmoId;
    Build.InstalledParts = {
        { ETUWeaponPartSlot::FireControl, FireControlPart.PartId },
        { ETUWeaponPartSlot::Trigger, TriggerPartA.PartId }
    };

    FString FailureReason;
    TestFalse(TEXT("Second simultaneous trigger is rejected"),
        FTUWeaponBuildEditor::TryInstallPart(
            Platform, TriggerPartB, Catalog->Parts, Build, FailureReason));

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

    TestTrue(TEXT("Trigger-equipped build configures"),
        Weapon->ConfigureFromCatalog(Catalog, Build, FailureReason));
    TestTrue(TEXT("Runtime weapon has dedicated trigger"), Weapon->HasActiveTrigger());
    TestEqual(TEXT("Runtime trigger identity is explicit"), Weapon->GetActiveTriggerId(), TriggerA.TriggerId);
    TestEqual(TEXT("Runtime trigger type is explicit"), Weapon->GetActiveTriggerType(), ETUTriggerType::TwoStage);
    TestEqual(TEXT("Dedicated trigger response overrides legacy fire-control fallback"),
        Weapon->GetTriggerResponseMultiplier(), TriggerA.TriggerResponseMultiplier);
    TestEqual(TEXT("Dedicated trigger reset overrides legacy fire-control fallback"),
        Weapon->GetResetResponseMultiplier(), TriggerA.ResetResponseMultiplier);
    TestEqual(TEXT("Dedicated trigger delay reaches runtime"),
        Weapon->GetSemiAutoResetDelaySeconds(), TriggerA.SemiAutoResetDelaySeconds);
    TestFalse(TEXT("Dedicated trigger release behavior reaches runtime"),
        Weapon->RequiresReleaseBetweenSemiShots());

    TestTrue(TEXT("Trigger can be atomically swapped"),
        FTUWeaponBuildEditor::TryReplacePart(
            Platform, TriggerPartA.PartId, TriggerPartB, Catalog->Parts, Build, FailureReason));
    TestTrue(TEXT("Swapped trigger build reconfigures"),
        Weapon->ConfigureFromCatalog(Catalog, Build, FailureReason));
    TestEqual(TEXT("Swapped trigger identity reaches runtime"), Weapon->GetActiveTriggerId(), TriggerB.TriggerId);
    TestEqual(TEXT("Swapped trigger type reaches runtime"), Weapon->GetActiveTriggerType(), ETUTriggerType::Match);
    TestEqual(TEXT("Swapped trigger response reaches runtime"),
        Weapon->GetTriggerResponseMultiplier(), TriggerB.TriggerResponseMultiplier);

    FWeaponBuildState MissingTriggerDefinitionBuild = Build;
    Catalog->Triggers.Reset();
    TestFalse(TEXT("Missing trigger definition rejects configuration"),
        Weapon->ConfigureFromCatalog(Catalog, MissingTriggerDefinitionBuild, FailureReason));
    TestEqual(TEXT("Rejected trigger configuration preserves previous trigger"),
        Weapon->GetActiveTriggerId(), TriggerB.TriggerId);

    World->DestroyWorld(false);
    return true;
}

#endif
