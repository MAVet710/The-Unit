#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TUEnemyAIController.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUAIContractTest,"TheUnit.AI.StateAndArchetypeContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUAIContractTest::RunTest(const FString&)
{
    TestEqual(TEXT("Seven lifecycle states"),(int32)ETUAIState::Dead+1,7);
    TestEqual(TEXT("Six archetypes"),(int32)ETUAIArchetype::Leader+1,6);
    ATUEnemyAIController* AI=NewObject<ATUEnemyAIController>();
    AI->ConfigureArchetype(ETUAIArchetype::Breacher); TestEqual(TEXT("Breacher identity"),AI->GetArchetype(),ETUAIArchetype::Breacher); TestTrue(TEXT("Breacher closes distance"),AI->GetPreferredEngageRangeCm()<1000.f);
    AI->ConfigureArchetype(ETUAIArchetype::Marksman); TestTrue(TEXT("Marksman holds long range"),AI->GetPreferredEngageRangeCm()>3000.f);
    AI->ConfigureArchetype(ETUAIArchetype::Scout); TestTrue(TEXT("Scout searches longer than rifleman"),AI->GetSearchDurationSeconds()>8.f);
    AI->ReportStimulus(FVector(100,0,0),false); TestEqual(TEXT("Audio produces suspicion"),AI->GetTacticalState(),ETUAIState::Suspicious);
    AI->ReportStimulus(FVector(100,0,0),true); TestEqual(TEXT("Visual contact engages"),AI->GetTacticalState(),ETUAIState::Engage);
    AI->LoseContact(); TestEqual(TEXT("Lost visual becomes search"),AI->GetTacticalState(),ETUAIState::Search);
    AI->MarkDead(); TestEqual(TEXT("Death terminal"),AI->GetTacticalState(),ETUAIState::Dead);
    AI->ReportStimulus(FVector::ZeroVector,true); TestEqual(TEXT("Dead AI ignores stimulus"),AI->GetTacticalState(),ETUAIState::Dead);
    return true;
}
#endif
