#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUFPVRadioInputComponent.generated.h"

class APlayerController;

USTRUCT(BlueprintType)
struct FFPVRadioAxisCalibration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    float RawMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    float RawCenter = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    float RawMax = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    bool bInvert = false;
};

USTRUCT(BlueprintType)
struct FFPVRadioChannels
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="FPV|Radio")
    float Throttle = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="FPV|Radio")
    float Roll = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="FPV|Radio")
    float Pitch = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="FPV|Radio")
    float Yaw = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="FPV|Radio")
    bool bValid = false;
};

/**
 * Device-agnostic RawInput adapter for USB RC transmitters.
 *
 * RawInput device/vendor configuration stays in Unreal's RawInput settings.
 * This component only translates four selected Generic USB axes into native
 * FPV channel semantics so the flight solver remains independent from HID APIs.
 */
UCLASS(ClassGroup=(FPV), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUFPVRadioInputComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUFPVRadioInputComponent();

    UFUNCTION(BlueprintCallable, Category="FPV|Radio")
    FFPVRadioChannels ReadChannels(APlayerController* PlayerController) const;

    UFUNCTION(BlueprintPure, Category="FPV|Radio")
    bool IsEnabled() const { return bEnabled; }

    UFUNCTION(BlueprintCallable, Category="FPV|Radio")
    void SetEnabled(bool bNewEnabled) { bEnabled = bNewEnabled; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    bool bEnabled = false;

    /** Generic USB axis numbers, 1-8. Defaults fit many Mode 2 RC HID layouts after RawInput remapping. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio", meta=(ClampMin="1", ClampMax="8"))
    int32 RollAxis = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio", meta=(ClampMin="1", ClampMax="8"))
    int32 PitchAxis = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio", meta=(ClampMin="1", ClampMax="8"))
    int32 ThrottleAxis = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio", meta=(ClampMin="1", ClampMax="8"))
    int32 YawAxis = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    FFPVRadioAxisCalibration RollCalibration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    FFPVRadioAxisCalibration PitchCalibration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    FFPVRadioAxisCalibration ThrottleCalibration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio")
    FFPVRadioAxisCalibration YawCalibration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio", meta=(ClampMin="0.0", ClampMax="0.25"))
    float CenterDeadband = 0.02f;

private:
    float ReadGenericAxis(APlayerController* PlayerController, int32 AxisNumber) const;
    float NormalizeCentered(float RawValue, const FFPVRadioAxisCalibration& Calibration) const;
    float NormalizeThrottle(float RawValue, const FFPVRadioAxisCalibration& Calibration) const;
};
