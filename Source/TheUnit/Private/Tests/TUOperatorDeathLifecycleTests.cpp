#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TUHealthComponent.h"
#include "TU_OperatorCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorDeathLifecycleTest,
    "TheUnit.Character.OperatorDeathLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorDeathLifecycleTest::RunTest(const FString& Parameters)
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

    ATU_OperatorCharacter* Operator = World->SpawnActor<ATU_OperatorCharacter>();
    if (!TestNotNull(TEXT("Operator actor"), Operator))
    {
        World->DestroyWorld(false);
        return false;
    }

    UTUHealthComponent* Health = Operator->GetHealthComponent();
    UCharacterMovementComponent* Movement = Operator->GetCharacterMovement();
    TestNotNull(TEXT("Health component"), Health);
    TestNotNull(TEXT("Movement component"), Movement);

    if (Health && Movement)
    {
        TestFalse(TEXT("Operator begins alive"), Operator->IsOperatorDead());
        Health->ApplyRegionalDamage(ETUBodyRegion::Head, 35.0f);
        TestTrue(TEXT("Lethal health state marks operator dead"), Operator->IsOperatorDead());
        TestEqual(TEXT("Death lifecycle disables character movement"), Movement->MovementMode, MOVE_None);

        Operator->StartSprint();
        Operator->StartCrouch();
        Operator->StartADS();
        Operator->StartLeanLeft();
        TestEqual(TEXT("Dead operator cannot re-enable movement mode through state inputs"),
            Movement->MovementMode, MOVE_None);

        Health->ApplyRegionalDamage(ETUBodyRegion::Chest, 999.0f);
        TestEqual(TEXT("Repeated lethal damage keeps movement disabled"), Movement->MovementMode, MOVE_None);
    }

    World->DestroyWorld(false);
    return true;
}

#endif
