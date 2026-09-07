#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TU_FPVDrone.generated.h"

class UBoxComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class ETUFPVFlightMode : uint8
{
    Acro UMETA(DisplayName = "Acro / Manual"),
    Stabilized UMETA(DisplayName = "Stabilized")
};

USTRUCT(BlueprintType)
struct FFPVPIDGains
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV")
    float P = 0.040f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV")
    float I = 0.010f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV")
    float D = 0.0012f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV")
    float FeedForward = 0.0f;
};

/**
 * Physics-first FPV quadcopter prototype.
 *
 * The vehicle is a rigid body driven by four independent rotor forces. Player
 * input creates angular-rate targets (Acro) or angle targets that are converted
 * to rate targets (Stabilized). PID outputs are mixed into per-motor commands.
 *
 * This class intentionally contains only the flight core. Battery sag,
 * propwash, damaged-prop behavior, radio/video signal simulation and full RC
 * transmitter support belong to later layers.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_FPVDrone : public APawn
{
    GENERATED_BODY()

public:
    ATU_FPVDrone();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "FPV|Flight")
    void ToggleFlightMode();

    UFUNCTION(BlueprintCallable, Category = "FPV|Flight")
    void ToggleArmed();

    UFUNCTION(BlueprintCallable, Category = "FPV|Flight")
    void ResetDrone();

    UFUNCTION(BlueprintPure, Category = "FPV|Flight")
    ETUFPVFlightMode GetFlightMode() const { return FlightMode; }

    UFUNCTION(BlueprintPure, Category = "FPV|Flight")
    bool IsArmed() const { return bArmed; }

    UFUNCTION(BlueprintPure, Category = "FPV|Telemetry")
    FVector4 GetMotorCommands() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Components")
    UBoxComponent* PhysicsBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Components")
    UCameraComponent* FPVCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Flight")
    ETUFPVFlightMode FlightMode = ETUFPVFlightMode::Stabilized;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Flight")
    bool bArmed = false;

    // 5-inch freestyle baseline. All values are intentionally exposed for tuning.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Airframe", meta = (ClampMin = "0.1"))
    float MassKg = 0.70f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Airframe", meta = (ClampMin = "1.0"))
    float ArmHalfSpanCm = 10.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Motors", meta = (ClampMin = "0.1"))
    float MaxMotorThrustNewton = 7.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Motors", meta = (ClampMin = "0.0"))
    float MotorResponseSeconds = 0.035f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Motors", meta = (ClampMin = "0.0"))
    float MaxReactionTorqueNm = 0.020f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Rates", meta = (ClampMin = "90.0", ClampMax = "2000.0"))
    float MaxRollRateDegPerSec = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Rates", meta = (ClampMin = "90.0", ClampMax = "2000.0"))
    float MaxPitchRateDegPerSec = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Rates", meta = (ClampMin = "90.0", ClampMax = "2000.0"))
    float MaxYawRateDegPerSec = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Rates", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RateExpo = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Stabilized", meta = (ClampMin = "5.0", ClampMax = "80.0"))
    float MaxLevelAngleDeg = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Stabilized", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float LevelRateGain = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|PID")
    FFPVPIDGains RollPID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|PID")
    FFPVPIDGains PitchPID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|PID")
    FFPVPIDGains YawPID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Aerodynamics", meta = (ClampMin = "0.0"))
    float AirDensityKgM3 = 1.225f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Aerodynamics", meta = (ClampMin = "0.0"))
    float DragCoefficient = 1.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Aerodynamics", meta = (ClampMin = "0.0"))
    FVector ProjectedAreaM2 = FVector(0.015f, 0.018f, 0.035f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|GroundEffect", meta = (ClampMin = "0.0"))
    float GroundEffectRangeCm = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|GroundEffect", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float GroundEffectStrength = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Camera", meta = (ClampMin = "0.0", ClampMax = "60.0"))
    float CameraTiltDeg = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Camera", meta = (ClampMin = "60.0", ClampMax = "160.0"))
    float CameraFOVDeg = 110.0f;

private:
    void InputThrottle(float Value);
    void InputRoll(float Value);
    void InputPitch(float Value);
    void InputYaw(float Value);

    void UpdateFlightController(float DeltaSeconds);
    void ApplyRotorForces(float DeltaSeconds);
    void ApplyAerodynamicDrag();
    float ComputeGroundEffectMultiplier() const;
    float ApplyRateCurve(float Input) const;
    float RunPID(const FFPVPIDGains& Gains, float Error, float TargetRate, float DeltaSeconds, float& Integral, float& PreviousError, float& PreviousTargetRate);
    void ClearControllerState();

    float ThrottleInput = 0.0f;
    float RollInput = 0.0f;
    float PitchInput = 0.0f;
    float YawInput = 0.0f;

    float DesiredMotorCommands[4] = { 0.f, 0.f, 0.f, 0.f };
    float MotorCommands[4] = { 0.f, 0.f, 0.f, 0.f };

    float RollIntegral = 0.0f;
    float PitchIntegral = 0.0f;
    float YawIntegral = 0.0f;
    float PreviousRollError = 0.0f;
    float PreviousPitchError = 0.0f;
    float PreviousYawError = 0.0f;
    float PreviousRollTarget = 0.0f;
    float PreviousPitchTarget = 0.0f;
    float PreviousYawTarget = 0.0f;

    FTransform SpawnTransform;
};
