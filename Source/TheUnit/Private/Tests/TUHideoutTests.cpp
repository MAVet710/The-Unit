#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUHideoutProgressionComponent.h"
#include "TU_HideoutCommandCenterDecorator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHideoutProgressionTest, "TheUnit.CommandCenter.Hideout.Progression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUHideoutProgressionTest::RunTest(const FString& Parameters)
{
    UTUHideoutProgressionComponent* Progression = NewObject<UTUHideoutProgressionComponent>();
    if (!TestNotNull(TEXT("Hideout progression component"), Progression))
    {
        return false;
    }

    TestEqual(TEXT("Eight hideout modules are registered"), Progression->GetModules().Num(), 8);
    TestEqual(TEXT("Power starts operational"), Progression->GetModuleLevel(ETUHideoutModuleType::Power), 1);
    TestEqual(TEXT("Armory support starts operational"), Progression->GetModuleLevel(ETUHideoutModuleType::ArmorySupport), 1);
    TestEqual(TEXT("Medical starts unbuilt"), Progression->GetModuleLevel(ETUHideoutModuleType::Medical), 0);
    TestFalse(TEXT("Unbuilt medical is not operational"), Progression->IsModuleOperational(ETUHideoutModuleType::Medical));

    TestTrue(TEXT("Medical can be upgraded"), Progression->UpgradeModule(ETUHideoutModuleType::Medical));
    TestEqual(TEXT("Medical reaches level one"), Progression->GetModuleLevel(ETUHideoutModuleType::Medical), 1);
    TestTrue(TEXT("Medical is operational after upgrade"), Progression->IsModuleOperational(ETUHideoutModuleType::Medical));

    TestTrue(TEXT("Level can jump to max"), Progression->SetModuleLevel(ETUHideoutModuleType::Medical, 3));
    TestFalse(TEXT("Max-level module cannot upgrade further"), Progression->UpgradeModule(ETUHideoutModuleType::Medical));
    TestEqual(TEXT("Level clamps high"), Progression->GetModuleLevel(ETUHideoutModuleType::Medical), 3);

    TestTrue(TEXT("Level can be clamped back to zero"), Progression->SetModuleLevel(ETUHideoutModuleType::Medical, -4));
    TestEqual(TEXT("Level clamps low"), Progression->GetModuleLevel(ETUHideoutModuleType::Medical), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHideoutDecoratorWiringTest, "TheUnit.CommandCenter.Hideout.DecoratorWiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUHideoutDecoratorWiringTest::RunTest(const FString& Parameters)
{
    const ATU_HideoutCommandCenterDecorator* CDO = GetDefault<ATU_HideoutCommandCenterDecorator>();
    if (!TestNotNull(TEXT("Hideout decorator CDO"), CDO))
    {
        return false;
    }

    TestNotNull(TEXT("Hideout decorator owns progression state"), CDO->GetProgression());
    return true;
}

#endif
