#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_WeaponBase.h"
#include "TUWeaponComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponOwnershipTest, "TheUnit.Combat.WeaponOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponOwnershipTest::RunTest(const FString& Parameters)
{
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

    TestEqual(TEXT("Initial loaded total"), Weapon->GetCurrentAmmo(), 30);
    TestEqual(TEXT("Magazine excludes chamber"), Weapon->GetMagazineState().RoundsInMagazine, 29);
    TestTrue(TEXT("Initial chamber"), Weapon->GetMagazineState().bRoundChambered);
    TestEqual(TEXT("Initial reserve"), Weapon->GetReserveAmmo(), 90);
    FMagazineState Snapshot = Weapon->GetMagazineState();
    Snapshot.RoundsInMagazine = 0;
    TestEqual(TEXT("Read API returns a snapshot, not mutable state"), Weapon->GetCurrentAmmo(), 30);
    TestFalse(TEXT("No actor tick"), Weapon->PrimaryActorTick.bCanEverTick);

    Weapon->FinishReload();
    TestEqual(TEXT("Finish without start does nothing"), Weapon->GetCurrentAmmo(), 30);
    Weapon->StartReload();
    TestEqual(TEXT("Tactical reload allows capacity plus chamber"), Weapon->GetCurrentAmmo(), 31);
    TestEqual(TEXT("Tactical reload conserves ammo"), Weapon->GetReserveAmmo(), 89);
    Weapon->StartReload();
    TestEqual(TEXT("Full reload is a no-op"), Weapon->GetReserveAmmo(), 89);

    for (int32 Remaining = 30; Remaining >= 0; --Remaining)
    {
        Weapon->Fire();
        TestEqual(TEXT("Each shot consumes exactly one round"), Weapon->GetCurrentAmmo(), Remaining);
    }
    TestFalse(TEXT("Empty weapon cannot fire"), Weapon->CanFire());
    Weapon->Fire();
    TestEqual(TEXT("Dry fire consumes nothing"), Weapon->GetCurrentAmmo(), 0);
    Weapon->StartReload();
    TestEqual(TEXT("Empty reload loads 30 total"), Weapon->GetCurrentAmmo(), 30);
    TestEqual(TEXT("Empty reload transfers reserve"), Weapon->GetReserveAmmo(), 59);
    Weapon->SetFireMode(ETUFireMode::Burst);
    Weapon->StartFire();
    TestEqual(TEXT("Burst uses shared ammunition"), Weapon->GetCurrentAmmo(), 27);
    Weapon->CycleFireMode();
    TestEqual(TEXT("Actor cycles mode"), Weapon->GetCurrentFireMode(), ETUFireMode::FullAuto);
    Weapon->StartFire();
    TestEqual(TEXT("Existing full-auto skeleton consumes one shot"), Weapon->GetCurrentAmmo(), 26);
    Weapon->StopFire();
    Weapon->AddReserveAmmo(-1);
    Weapon->AddReserveAmmo(0);
    TestEqual(TEXT("Nonpositive additions ignored"), Weapon->GetReserveAmmo(), 59);
    Weapon->AddReserveAmmo(MAX_int32);
    TestEqual(TEXT("Reserve addition saturates safely"), Weapon->GetReserveAmmo(), MAX_int32);

    UTUWeaponComponent* Mechanics = Weapon->FindComponentByClass<UTUWeaponComponent>();
    if (TestNotNull(TEXT("Actor owns mechanics subobject"), Mechanics))
    {
        TestFalse(TEXT("No mechanics tick"), Mechanics->PrimaryComponentTick.bCanEverTick);
        TestNull(TEXT("Component has no public Blueprint firing API"),
            Mechanics->FindFunction(TEXT("FireSemiAuto")));
        // Exercise editor-authored chamber states through reflected defaults, not a public mutation API.
        FStructProperty* MagazineProperty = FindFProperty<FStructProperty>(Mechanics->GetClass(), TEXT("MagazineState"));
        FIntProperty* ReserveProperty = FindFProperty<FIntProperty>(Mechanics->GetClass(), TEXT("AmmoReserve"));
        if (TestNotNull(TEXT("Magazine defaults"), MagazineProperty)
            && TestNotNull(TEXT("Reserve defaults"), ReserveProperty))
        {
            TestFalse(TEXT("Blueprint cannot write magazine internals"), MagazineProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
            FMagazineState* Magazine = MagazineProperty->ContainerPtrToValuePtr<FMagazineState>(Mechanics);
            Magazine->RoundsInMagazine = 2;
            Magazine->bRoundChambered = false;
            Weapon->SetFireMode(ETUFireMode::SemiAuto);
            Weapon->Fire();
            TestEqual(TEXT("Unchambered shot consumes one, not zero"), Weapon->GetCurrentAmmo(), 1);
            Weapon->Fire();
            TestEqual(TEXT("Last chambered round is consumed"), Weapon->GetCurrentAmmo(), 0);
            Magazine->RoundsInMagazine = 1;
            Weapon->Fire();
            TestEqual(TEXT("Single unchambered round is consumed"), Weapon->GetCurrentAmmo(), 0);
            ReserveProperty->SetPropertyValue_InContainer(Mechanics, 2);
            Weapon->StartReload();
            TestEqual(TEXT("Partial reserve loads only available rounds"), Weapon->GetCurrentAmmo(), 2);
            TestEqual(TEXT("Partial reserve exhausted"), Weapon->GetReserveAmmo(), 0);
            Weapon->StartReload();
            TestEqual(TEXT("No reserve reload is a no-op"), Weapon->GetCurrentAmmo(), 2);
        }
        FStructProperty* DefinitionProperty = FindFProperty<FStructProperty>(Mechanics->GetClass(), TEXT("WeaponDefinition"));
        if (TestNotNull(TEXT("Weapon definition defaults"), DefinitionProperty))
        {
            DefinitionProperty->ContainerPtrToValuePtr<FWeaponDefinition>(Mechanics)->bSemiAutoOnly = true;
            Weapon->SetFireMode(ETUFireMode::Burst);
            TestEqual(TEXT("Actor enforces definition mode constraint"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);
            Weapon->CycleFireMode();
            TestEqual(TEXT("Semi-only weapon stays in semi-auto"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);
        }
    }
    World->DestroyWorld(false);
    return true;
}

#endif
