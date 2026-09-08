#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_Karambit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUKarambitRuntimeTest, "TheUnit.Combat.Karambit.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUKarambitRuntimeTest::RunTest(const FString& Parameters)
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

    ATU_Karambit* Knife = World->SpawnActor<ATU_Karambit>();
    if (!TestNotNull(TEXT("Karambit actor"), Knife))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestNotNull(TEXT("Karambit owns handle mesh"), Knife->GetHandleMesh());
    TestNotNull(TEXT("Karambit owns fixed blade mesh"), Knife->GetBladeMesh());
    TestEqual(TEXT("Karambit uses shared initial readiness state"),
        Knife->GetBladeState(), ETUOTFBladeState::Retracted);
    TestTrue(TEXT("Karambit fixed-blade draw delay remains short"),
        Knife->GetRetractionDurationSeconds() <= 0.05f + KINDA_SMALL_NUMBER);

    Knife->DeployBlade();
    Knife->Tick(1.0f);
    TestTrue(TEXT("Karambit becomes melee-ready after draw presentation"), Knife->IsBladeDeployed());

    Knife->RetractBlade();
    Knife->Tick(1.0f);
    TestEqual(TEXT("Karambit returns to holstered readiness state"),
        Knife->GetBladeState(), ETUOTFBladeState::Retracted);
    TestFalse(TEXT("Holstered karambit cannot melee-hit"), Knife->PerformMeleeAttack());

    World->DestroyWorld(false);
    return true;
}

#endif
