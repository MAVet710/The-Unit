#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_OTFKnife.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOTFKnifeRuntimeTest, "TheUnit.Combat.OTFKnife.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOTFKnifeRuntimeTest::RunTest(const FString& Parameters)
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

    ATU_OTFKnife* Knife = World->SpawnActor<ATU_OTFKnife>();
    if (!TestNotNull(TEXT("OTF knife actor"), Knife))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("Knife begins retracted"), Knife->GetBladeState(), ETUOTFBladeState::Retracted);
    TestFalse(TEXT("Knife does not begin deployed"), Knife->IsBladeDeployed());
    TestEqual(TEXT("Retracted blade alpha"), Knife->GetBladeAlpha(), 0.0f);
    TestNotNull(TEXT("Knife owns handle mesh"), Knife->GetHandleMesh());
    TestNotNull(TEXT("Knife owns independent blade mesh"), Knife->GetBladeMesh());

    Knife->DeployBlade();
    TestEqual(TEXT("Deploy enters deploying state"), Knife->GetBladeState(), ETUOTFBladeState::Deploying);
    Knife->Tick(1.0f);
    TestTrue(TEXT("Blade reaches deployed state"), Knife->IsBladeDeployed());
    TestEqual(TEXT("Deployed blade alpha"), Knife->GetBladeAlpha(), 1.0f);

    Knife->RetractBlade();
    TestEqual(TEXT("Retract enters retracting state"), Knife->GetBladeState(), ETUOTFBladeState::Retracting);
    Knife->Tick(1.0f);
    TestEqual(TEXT("Blade returns retracted"), Knife->GetBladeState(), ETUOTFBladeState::Retracted);
    TestEqual(TEXT("Retracted blade alpha after cycle"), Knife->GetBladeAlpha(), 0.0f);

    TestFalse(TEXT("Retracted blade cannot perform melee hit"), Knife->PerformMeleeAttack());

    World->DestroyWorld(false);
    return true;
}

#endif
