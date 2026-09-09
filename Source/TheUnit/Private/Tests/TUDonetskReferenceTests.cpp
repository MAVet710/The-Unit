#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TU_DonetskDistrictGenerator.h"
#include "TU_DonetskMissionGameMode.h"
#include "TU_GameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUDonetskGeneratorDefaultsTest,
    "TheUnit.Maps.Donetsk.ReferenceGenerator.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUDonetskGeneratorDefaultsTest::RunTest(const FString& Parameters)
{
    const ATU_DonetskDistrictGenerator* CDO = GetDefault<ATU_DonetskDistrictGenerator>();
    if (!TestNotNull(TEXT("Donetsk district generator CDO"), CDO))
    {
        return false;
    }

    TestTrue(TEXT("Donetsk reference generator is an actor"), CDO->IsA<AActor>());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUDonetskGameModeWiringTest,
    "TheUnit.Maps.Donetsk.GameMode.Wiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUDonetskGameModeWiringTest::RunTest(const FString& Parameters)
{
    const ATU_DonetskMissionGameMode* CDO = GetDefault<ATU_DonetskMissionGameMode>();
    if (!TestNotNull(TEXT("Donetsk GameMode CDO"), CDO))
    {
        return false;
    }

    TestTrue(TEXT("Donetsk mission GameMode inherits The Unit GameMode"), CDO->IsA<ATU_GameMode>());
    return true;
}

#endif
