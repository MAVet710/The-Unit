#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TUHealthComponent.h"
#include "TU_OperatorCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorHealthIntegrationTest,
    "TheUnit.Character.OperatorHealthIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorHealthIntegrationTest::RunTest(const FString& Parameters)
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
    TestNotNull(TEXT("Operator owns reusable health component"), Health);
    TestEqual(TEXT("Character exposes the same owned component"),
        Operator->FindComponentByClass<UTUHealthComponent>(), Health);
    TestFalse(TEXT("Fresh operator is alive"), Operator->IsOperatorDead());

    if (Health)
    {
        const float InitialTotal = Health->GetTotalHealth();
        Health->ApplyRegionalDamage(ETUBodyRegion::LeftArm, 10.0f);
        TestEqual(TEXT("Regional damage flows through owned health component"),
            Health->GetTotalHealth(), InitialTotal - 10.0f);
        TestFalse(TEXT("Nonlethal limb damage keeps operator alive"), Operator->IsOperatorDead());

        Health->ApplyRegionalDamage(ETUBodyRegion::Head, 35.0f);
        TestTrue(TEXT("Lethal regional health state is visible through operator API"), Operator->IsOperatorDead());
    }

    World->DestroyWorld(false);
    return true;
}

#endif
