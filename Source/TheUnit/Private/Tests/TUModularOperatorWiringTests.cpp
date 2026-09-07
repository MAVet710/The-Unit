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

    const ATU_ModularOperatorCharacter* OperatorDefaults = GetDefault<ATU_ModularOperatorCharacter>();
    if (!TestNotNull(TEXT("Modular operator defaults"), OperatorDefaults))
    {
        return false;
    }

    TestNotNull(TEXT("Modular operator owns equipment component"), OperatorDefaults->GetOperatorEquipment());
    TestNotNull(TEXT("Modular operator owns armor protection component"), OperatorDefaults->GetArmorProtection());
    TestNotNull(TEXT("Modular operator owns regional health component"), OperatorDefaults->GetOperatorHealth());

    return true;
}

#endif
