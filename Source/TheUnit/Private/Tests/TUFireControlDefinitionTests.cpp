#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TheUnitTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFireControlDefinitionTest,
    "TheUnit.Combat.FireControlDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUFireControlDefinitionTest::RunTest(const FString& Parameters)
{
    FFireControlModuleDefinition Definition;

    TestFalse(TEXT("Fire-control definition has stable identity"), Definition.FireControlId.IsNone());
    TestFalse(TEXT("Trigger profile has stable identity"), Definition.TriggerProfileId.IsNone());
    TestEqual(TEXT("Default fire control exposes one mode"), Definition.SupportedFireModes.Num(), 1);
    TestTrue(TEXT("Default fire control supports semi-auto"),
        Definition.SupportedFireModes.Contains(ETUFireMode::SemiAuto));
    TestFalse(TEXT("Default fire control does not implicitly support burst"),
        Definition.SupportedFireModes.Contains(ETUFireMode::Burst));
    TestFalse(TEXT("Default fire control does not implicitly support full-auto"),
        Definition.SupportedFireModes.Contains(ETUFireMode::FullAuto));
    TestEqual(TEXT("Default burst count remains explicit"), Definition.BurstCount, 3);
    TestEqual(TEXT("Neutral trigger response"), Definition.TriggerResponseMultiplier, 1.0f);
    TestEqual(TEXT("Neutral reset response"), Definition.ResetResponseMultiplier, 1.0f);
    TestTrue(TEXT("Semi-auto release requirement defaults on"),
        Definition.bRequiresReleaseBetweenSemiShots);

    FWeaponPartDefinition FireControlPart;
    FireControlPart.PartId = TEXT("Part_Test_FireControl");
    FireControlPart.Slot = ETUWeaponPartSlot::FireControl;
    FireControlPart.FireControlDefinitionId = Definition.FireControlId;

    TestTrue(TEXT("Fire-control part uses the dedicated slot"),
        FireControlPart.Slot == ETUWeaponPartSlot::FireControl);
    TestEqual(TEXT("Fire-control part references behavior definition"),
        FireControlPart.FireControlDefinitionId, Definition.FireControlId);

    Definition.SupportedFireModes = {
        ETUFireMode::SemiAuto,
        ETUFireMode::Burst,
        ETUFireMode::FullAuto
    };
    Definition.TriggerProfileId = TEXT("Trigger.Test.SelectFire");
    Definition.BurstCount = 2;
    Definition.TriggerResponseMultiplier = 0.9f;
    Definition.ResetResponseMultiplier = 0.8f;

    TestEqual(TEXT("Custom fire-control mode count"), Definition.SupportedFireModes.Num(), 3);
    TestTrue(TEXT("Custom fire control can expose burst"),
        Definition.SupportedFireModes.Contains(ETUFireMode::Burst));
    TestTrue(TEXT("Custom fire control can expose full-auto"),
        Definition.SupportedFireModes.Contains(ETUFireMode::FullAuto));
    TestEqual(TEXT("Custom burst count is data-driven"), Definition.BurstCount, 2);

    return true;
}

#endif
