#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUFPVBatteryComponent.h"
#include "TUFPVRadioInputComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFPVBatterySimulationTest, "TheUnit.FPV.BatterySimulation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUFPVBatterySimulationTest::RunTest(const FString& Parameters)
{
    UTUFPVBatteryComponent* Battery = NewObject<UTUFPVBatteryComponent>();
    if (!TestNotNull(TEXT("Battery component"), Battery))
    {
        return false;
    }

    TestTrue(TEXT("Fresh 6S pack starts near 25.2 V"), FMath::IsNearlyEqual(Battery->GetVoltage(), 25.2f, 0.01f));
    TestTrue(TEXT("Fresh battery begins full"), FMath::IsNearlyEqual(Battery->GetBatteryPercent(), 100.0f, 0.01f));
    TestFalse(TEXT("Fresh pack is not low voltage"), Battery->IsLowVoltage());

    Battery->ConsumeCurrent(100.0f, 0.0f);
    const float SaggedVoltage = Battery->GetVoltage();
    TestTrue(TEXT("Heavy current causes immediate voltage sag"), SaggedVoltage < 25.2f);
    TestTrue(TEXT("Zero-time sag does not consume capacity"), FMath::IsNearlyEqual(Battery->GetBatteryPercent(), 100.0f, 0.01f));

    Battery->ConsumeCurrent(0.0f, 0.0f);
    TestTrue(TEXT("Removing load recovers open-circuit voltage"), Battery->GetVoltage() > SaggedVoltage);

    Battery->ConsumeCurrent(20.0f, 60.0f);
    TestTrue(TEXT("Sustained current consumes state of charge"), Battery->GetBatteryPercent() < 100.0f);
    TestTrue(TEXT("Loaded battery has less thrust headroom"), Battery->GetThrustScale() < 1.0f);

    Battery->ConsumeCurrent(100.0f, 60.0f);
    TestTrue(TEXT("Deeply discharged pack reports low voltage"), Battery->IsLowVoltage());
    TestTrue(TEXT("Thrust scale remains bounded"), Battery->GetThrustScale() >= Battery->MinimumThrustScale);

    Battery->ResetBattery();
    TestTrue(TEXT("Service reset restores charge"), FMath::IsNearlyEqual(Battery->GetBatteryPercent(), 100.0f, 0.01f));
    TestTrue(TEXT("Service reset restores fresh voltage"), FMath::IsNearlyEqual(Battery->GetVoltage(), 25.2f, 0.01f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFPVRadioSafetyTest, "TheUnit.FPV.RadioSafetyDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUFPVRadioSafetyTest::RunTest(const FString& Parameters)
{
    UTUFPVRadioInputComponent* Radio = NewObject<UTUFPVRadioInputComponent>();
    if (!TestNotNull(TEXT("Radio input component"), Radio))
    {
        return false;
    }

    TestFalse(TEXT("USB RC input is opt-in"), Radio->IsEnabled());
    TestEqual(TEXT("Default roll axis"), Radio->RollAxis, 1);
    TestEqual(TEXT("Default pitch axis"), Radio->PitchAxis, 2);
    TestEqual(TEXT("Default throttle axis"), Radio->ThrottleAxis, 3);
    TestEqual(TEXT("Default yaw axis"), Radio->YawAxis, 4);

    const FFPVRadioChannels DisabledChannels = Radio->ReadChannels(nullptr);
    TestFalse(TEXT("Disabled/null radio never reports valid channels"), DisabledChannels.bValid);
    TestTrue(TEXT("Disabled radio throttle is safe zero"), FMath::IsNearlyZero(DisabledChannels.Throttle));

    Radio->SetEnabled(true);
    const FFPVRadioChannels MissingControllerChannels = Radio->ReadChannels(nullptr);
    TestFalse(TEXT("Missing controller cannot become a valid RC source"), MissingControllerChannels.bValid);
    TestTrue(TEXT("Missing controller throttle remains safe zero"), FMath::IsNearlyZero(MissingControllerChannels.Throttle));

    return true;
}

#endif
