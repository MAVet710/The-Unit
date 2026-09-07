#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_CommandCenterGenerator.h"
#include "TU_CommandCenterStation.h"
#include "TU_RangeTarget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUCommandCenterStationDataTest, "TheUnit.CommandCenter.StationData",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUCommandCenterStationDataTest::RunTest(const FString& Parameters)
{
    const ATU_CommandCenterStation* DefaultStation = GetDefault<ATU_CommandCenterStation>();
    if (!TestNotNull(TEXT("Command center station CDO"), DefaultStation))
    {
        return false;
    }

    TestEqual(TEXT("Station defaults to Armory routing"), DefaultStation->GetStationType(), ETUCommandCenterStationType::Armory);
    TestFalse(TEXT("Default station label is not empty"), DefaultStation->GetStationLabel().IsEmpty());

    const ATU_ArmedOperatorCharacter* DefaultOperator = GetDefault<ATU_ArmedOperatorCharacter>();
    if (!TestNotNull(TEXT("Armed operator CDO"), DefaultOperator))
    {
        return false;
    }

    TestFalse(TEXT("MX50 begins stowed on the chest"), DefaultOperator->IsMX50Raised());
    TestEqual(TEXT("MX50 uses the chest-rig tablet socket"), DefaultOperator->GetMX50ChestSocket(), FName(TEXT("tablet_chest_socket")));

    const ATU_RangeTarget* DefaultTarget = GetDefault<ATU_RangeTarget>();
    if (!TestNotNull(TEXT("Range target CDO"), DefaultTarget))
    {
        return false;
    }

    TestTrue(TEXT("Range target begins with positive health"), DefaultTarget->GetCurrentHealth() > 0.0f);
    TestEqual(TEXT("Range target begins with zero hits"), DefaultTarget->GetHitCount(), 0);
    TestEqual(TEXT("Range target begins with zero resets"), DefaultTarget->GetResetCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUCommandCenterGrayboxRuntimeTest, "TheUnit.CommandCenter.GrayboxRuntime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUCommandCenterGrayboxRuntimeTest::RunTest(const FString& Parameters)
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

    ATU_CommandCenterGenerator* Generator = World->SpawnActor<ATU_CommandCenterGenerator>();
    if (!TestNotNull(TEXT("Command center generator"), Generator))
    {
        World->DestroyWorld(false);
        return false;
    }

    TArray<UActorComponent*> Components;
    Generator->GetComponents(Components);
    TestTrue(TEXT("Graybox generator creates substantial room/corridor component set"), Components.Num() > 30);

    ATU_CommandCenterStation* Station = World->SpawnActor<ATU_CommandCenterStation>();
    TestNotNull(TEXT("Runtime station actor"), Station);
    if (Station)
    {
        Station->ConfigureStation(
            ETUCommandCenterStationType::Briefing,
            FText::FromString(TEXT("BRIEFING // RAISE CHEST-MOUNTED MX50")),
            TEXT("OP_KILLHOUSE"));
        TestEqual(TEXT("Station can route as Briefing"), Station->GetStationType(), ETUCommandCenterStationType::Briefing);
        TestEqual(TEXT("Configured MX50 briefing label is preserved"), Station->GetStationLabel().ToString(), FString(TEXT("BRIEFING // RAISE CHEST-MOUNTED MX50")));
    }

    ATU_RangeTarget* Target = World->SpawnActor<ATU_RangeTarget>();
    TestNotNull(TEXT("Runtime range target"), Target);
    if (Target)
    {
        FDamageEvent DamageEvent;
        const float Applied = Target->TakeDamage(25.0f, DamageEvent, nullptr, nullptr);
        TestEqual(TEXT("Range target reports applied damage"), Applied, 25.0f);
        TestTrue(TEXT("Range target health decreases"), Target->GetCurrentHealth() < 100.0f);
        TestEqual(TEXT("Range target records hit"), Target->GetHitCount(), 1);

        Target->ResetTarget();
        TestEqual(TEXT("Manual reset restores target health"), Target->GetCurrentHealth(), 100.0f);
        TestEqual(TEXT("Manual reset increments reset counter"), Target->GetResetCount(), 1);
    }

    World->DestroyWorld(false);
    return true;
}

#endif
