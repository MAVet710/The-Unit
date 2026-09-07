#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TU_GameMode.h"
#include "TU_ModularOperatorCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUModularOperatorGameModeWiringTest, "TheUnit.Operator.Equipment.GameModeWiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUModularOperatorGameModeWiringTest::RunTest(const FString& Parameters)
{
    const ATU_GameMode* GameModeDefaults = GetDefault<ATU_GameMode>();
    if (!TestNotNull(TEXT("The Unit game mode defaults"), GameModeDefaults))
    {
        return false;
    }

    if (!TestNotNull(TEXT("Game mode has a default pawn class"), GameModeDefaults->DefaultPawnClass))
    {
        return false;
    }

    TestTrue(TEXT("The Unit game mode spawns the modular operator"),
        GameModeDefaults->DefaultPawnClass->IsChildOf(ATU_ModularOperatorCharacter::StaticClass()));

    return true;
}

#endif
