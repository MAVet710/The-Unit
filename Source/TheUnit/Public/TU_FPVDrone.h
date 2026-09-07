#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TU_FPVDrone.generated.h"

class UBoxComponent;
class UCameraComponent;
class UPrimitiveComponent;
class UTUFPVBatteryComponent;
class UTUFPVSignalComponent;

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

USTRUCT(BlueprintType)
struct FFPVTelemetry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float BatteryVoltage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float BatteryPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float CurrentDrawAmps = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float VideoQuality = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float ControlQuality = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float VideoLatencyMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float LinkDistanceMeters = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float SpeedMetersPerSecond = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float AltitudeAGLMeters = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    float PropwashIntensity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    FVector4 MotorHealth = FVector4(1.0, 1.0, 1.0, 1.0);

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    ETUFPVFlightMode FlightMode = ETUFPVFlightMode::Stabilized;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    bool bArmed = false;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    bool bLowBattery = false;

    UPROPERTY(BlueprintReadOnly, Category = "FPV|Telemetry")
    bool bSignalFailsafe = false;
};

/**
 * Physics-first FPV quadcopter prototype.
 *
 * Four independent rotor forces drive a Chaos rigid body. Acro commands angular
 * rates; Stabilized mode converts angle targets to rate targets. Phase 2 adds a
 * lightweight LiPo model, rotor-wake/propwash loss, per-motor impact damage,
 * signal/link telemetry, and an external-RC input seam while keeping those
 * systems independently tunable.
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

    /** Prototype pit/bench action: recharge the pack and repair all props. */
    UFUNCTION(BlueprintCallable, Category = "FPV|Service")
    void ServiceDrone();

    UFUNCTION(BlueprintCallable, Category = "FPV|Possession")
    void SetReturnPawn(APawn* PawnToReturnTo);

    UFUNCTION(BlueprintCallable, Category = "FPV|Possession")
    void ExitDrone();

    /** Enables a future RawInput/Enhanced Input adapter to feed native RC values. */
    UFUNCTION(BlueprintCallable, Category = "FPV|Input")
    void SetExternalRCEnabled(bool bEnabled);

    /** Native RC semantics: throttle [0..1], other axes [-1..1]. */
    UFUNCTION(BlueprintCallable, Category = "FPV|Input")
    void SetExternalRCInput(float Throttle01, float Roll, float Pitch, float Yaw);

    UFUNCTION(BlueprintPure, Category = "FPV|Flight")
    ETUFPVFlightMode GetFlightMode() const { return FlightMode; }

    UFUNCTION(BlueprintPure, Category = "FPV|Flight")
    bool IsArmed() const { return bArmed; }

    UFUNCTION(BlueprintPure, Category = "FPV|Telemetry")
    FVector4 GetMotorCommands() const;

    UFUNCTION(BlueprintPure, Category = "FPV|Telemetry")
    FVector4 GetMotorHealth() const;

    UFUNCTION(BlueprintPure, Category = "FPV|Telemetry")
    FFPVTelemetry GetTelemetry() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Components")
    TObjectPtr<UBoxComponent> PhysicsBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Components")
    TObjectPtr<UCameraComponent> FPVCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Components")
    TObjectPtr<UTUFPVBatteryComponent> Battery;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Components")
    TObjectPtr<UTUFPVSignalComponent> Signal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Flight")
    ETUFPVFlightMode FlightMode = ETUFPVFlightMode::Stabilized;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Flight")
    bool bArmed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Flight")
    bool bEnableSignalFailsafe = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Flight", meta = (ClampMin = "0.0"))
    float SignalFailsafeDisarmSeconds = 1.0f;

    // 5-inch freestyle baseline. All values remain exposed for measured tuning.
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

    /** Approximate maximum electrical current per motor at full command. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Motors", meta = (ClampMin = "0.0"))
    float MaxMotorCurrentAmps = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Motors", meta = (ClampMin = "0.0"))
    float ElectronicsIdleCurrentAmps = 0.45f;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Propwash", meta = (ClampMin = "0.0"))
    float PropwashStartDescentMps = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Propwash", meta = (ClampMin = "0.1"))
    float PropwashFullDescentMps = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Propwash", meta = (ClampMin = "0.1"))
    float PropwashLateralEscapeMps = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Propwash", meta = (ClampMin = "0.0", ClampMax = "0.6"))
    float PropwashMaxThrustLoss = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Propwash", meta = (ClampMin = "0.0"))
    float PropwashDisturbanceTorqueNm = 0.012f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Damage", meta = (ClampMin = "0.0"))
    float DamageStartDeltaVMps = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Damage", meta = (ClampMin = "0.1"))
    float DamageFullDeltaVMps = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxMotorDamagePerImpact = 0.70f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Damage", meta = (ClampMin = "0.0"))
    float DamagedPropVibrationTorqueNm = 0.006f;

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
    void UpdateBattery(float DeltaSeconds);
    void UpdateSignalFailsafe(float DeltaSeconds);
    void ApplyPropwashDisturbance(float PropwashIntensity);
    void ApplyDamagedPropVibration();
    float ComputeGroundEffectMultiplier() const;
    float ComputePropwashIntensity() const;
    float ComputeAltitudeAGLMeters() const;
    float ApplyRateCurve(float Input) const;
    float RunPID(const FFPVPIDGains& Gains, float Error, float TargetRate, float DeltaSeconds, float& Integral, float& PreviousError, float& PreviousTargetRate);
    void ClearControllerState();

    UFUNCTION()
    void HandlePhysicsHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UPROPERTY()
    TObjectPtr<APawn> ReturnPawn = nullptr;

    bool bExternalRCInputEnabled = false;
    bool bSignalFailsafeActive = false;
    float SignalFailsafeElapsed = 0.0f;

    float ThrottleInput = 0.0f;
    float RollInput = 0.0f;
    float PitchInput = 0.0f;
    float YawInput = 0.0f;

    float DesiredMotorCommands[4] = { 0.f, 0.f, 0.f, 0.f };
    float MotorCommands[4] = { 0.f, 0.f, 0.f, 0.f };
    float MotorHealth[4] = { 1.f, 1.f, 1.f, 1.f };

    float CurrentPropwashIntensity = 0.0f;
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
