#include "TUFPVRadioInputComponent.h"

#include "GameFramework/PlayerController.h"
#include "RawInputFunctionLibrary.h"

UTUFPVRadioInputComponent::UTUFPVRadioInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FFPVRadioChannels UTUFPVRadioInputComponent::ReadChannels(APlayerController* PlayerController) const
{
    FFPVRadioChannels Channels;
    if (!bEnabled || !PlayerController)
    {
        return Channels;
    }

    const TArray<FRegisteredDeviceInfo> Devices = URawInputFunctionLibrary::GetRegisteredDevices();
    if (Devices.IsEmpty())
    {
        return Channels;
    }

    const float RawRoll = ReadGenericAxis(PlayerController, RollAxis);
    const float RawPitch = ReadGenericAxis(PlayerController, PitchAxis);
    const float RawThrottle = ReadGenericAxis(PlayerController, ThrottleAxis);
    const float RawYaw = ReadGenericAxis(PlayerController, YawAxis);

    Channels.Roll = NormalizeCentered(RawRoll, RollCalibration);
    Channels.Pitch = NormalizeCentered(RawPitch, PitchCalibration);
    Channels.Throttle = NormalizeThrottle(RawThrottle, ThrottleCalibration);
    Channels.Yaw = NormalizeCentered(RawYaw, YawCalibration);
    Channels.bValid = true;
    return Channels;
}

float UTUFPVRadioInputComponent::ReadGenericAxis(APlayerController* PlayerController, int32 AxisNumber) const
{
    if (!PlayerController)
    {
        return 0.0f;
    }

    switch (FMath::Clamp(AxisNumber, 1, 8))
    {
        case 1: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis1);
        case 2: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis2);
        case 3: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis3);
        case 4: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis4);
        case 5: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis5);
        case 6: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis6);
        case 7: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis7);
        case 8: return PlayerController->GetInputAnalogKeyState(FRawInputKeys::GenericUSBController_Axis8);
        default: return 0.0f;
    }
}

float UTUFPVRadioInputComponent::NormalizeCentered(float RawValue, const FFPVRadioAxisCalibration& Calibration) const
{
    const float MinValue = FMath::Min(Calibration.RawMin, Calibration.RawMax);
    const float MaxValue = FMath::Max(Calibration.RawMin, Calibration.RawMax);
    const float Center = FMath::Clamp(Calibration.RawCenter, MinValue, MaxValue);
    const float Clamped = FMath::Clamp(RawValue, MinValue, MaxValue);

    float Normalized = 0.0f;
    if (Clamped >= Center)
    {
        const float Denominator = FMath::Max(MaxValue - Center, KINDA_SMALL_NUMBER);
        Normalized = (Clamped - Center) / Denominator;
    }
    else
    {
        const float Denominator = FMath::Max(Center - MinValue, KINDA_SMALL_NUMBER);
        Normalized = -((Center - Clamped) / Denominator);
    }

    if (FMath::Abs(Normalized) < CenterDeadband)
    {
        Normalized = 0.0f;
    }

    if (Calibration.bInvert)
    {
        Normalized *= -1.0f;
    }

    return FMath::Clamp(Normalized, -1.0f, 1.0f);
}

float UTUFPVRadioInputComponent::NormalizeThrottle(float RawValue, const FFPVRadioAxisCalibration& Calibration) const
{
    const float MinValue = FMath::Min(Calibration.RawMin, Calibration.RawMax);
    const float MaxValue = FMath::Max(Calibration.RawMin, Calibration.RawMax);
    const float Denominator = FMath::Max(MaxValue - MinValue, KINDA_SMALL_NUMBER);
    float Normalized = (FMath::Clamp(RawValue, MinValue, MaxValue) - MinValue) / Denominator;

    if (Calibration.bInvert)
    {
        Normalized = 1.0f - Normalized;
    }

    return FMath::Clamp(Normalized, 0.0f, 1.0f);
}
