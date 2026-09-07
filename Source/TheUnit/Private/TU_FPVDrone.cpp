#include "TU_FPVDrone.h"

#include "TUFPVBatteryComponent.h"
#include "TUFPVSignalComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"

namespace
{
    constexpr float NewtonToUnrealForce = 100.0f;
    constexpr float NewtonMeterToUnrealTorque = 10000.0f;
    constexpr float CmPerMeter = 100.0f;
}

ATU_FPVDrone::ATU_FPVDrone()
{
    PrimaryActorTick.bCanEverTick = true;

    PhysicsBody = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsBody"));
    SetRootComponent(PhysicsBody);
    PhysicsBody->SetBoxExtent(FVector(16.0f, 16.0f, 4.0f));
    PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PhysicsBody->SetCollisionObjectType(ECC_PhysicsBody);
    PhysicsBody->SetCollisionResponseToAllChannels(ECR_Block);
    PhysicsBody->SetSimulatePhysics(true);
    PhysicsBody->SetEnableGravity(true);
    PhysicsBody->SetLinearDamping(0.02f);
    PhysicsBody->SetAngularDamping(0.01f);
    PhysicsBody->SetNotifyRigidBodyCollision(true);
    PhysicsBody->OnComponentHit.AddDynamic(this, &ATU_FPVDrone::HandlePhysicsHit);

    FPVCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPVCamera"));
    FPVCamera->SetupAttachment(PhysicsBody);
    FPVCamera->SetRelativeLocation(FVector(11.0f, 0.0f, 2.0f));

    Battery = CreateDefaultSubobject<UTUFPVBatteryComponent>(TEXT("Battery"));
    Signal = CreateDefaultSubobject<UTUFPVSignalComponent>(TEXT("Signal"));

    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ATU_FPVDrone::BeginPlay()
{
    Super::BeginPlay();

    SpawnTransform = GetActorTransform();
    PhysicsBody->SetMassOverrideInKg(NAME_None, MassKg, true);
    FPVCamera->SetRelativeRotation(FRotator(CameraTiltDeg, 0.0f, 0.0f));
    FPVCamera->SetFieldOfView(CameraFOVDeg);
}

void ATU_FPVDrone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!PhysicsBody || DeltaSeconds <= SMALL_NUMBER)
    {
        return;
    }

    UpdateSignalFailsafe(DeltaSeconds);

    if (bArmed && !bSignalFailsafeActive)
    {
        UpdateFlightController(DeltaSeconds);
    }
    else
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            DesiredMotorCommands[Index] = 0.0f;
        }
    }

    CurrentPropwashIntensity = ComputePropwashIntensity();
    ApplyRotorForces(DeltaSeconds);
    ApplyAerodynamicDrag();
    UpdateBattery(DeltaSeconds);
}

void ATU_FPVDrone::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("FPVThrottle"), this, &ATU_FPVDrone::InputThrottle);
    PlayerInputComponent->BindAxis(TEXT("FPVRoll"), this, &ATU_FPVDrone::InputRoll);
    PlayerInputComponent->BindAxis(TEXT("FPVPitch"), this, &ATU_FPVDrone::InputPitch);
    PlayerInputComponent->BindAxis(TEXT("FPVYaw"), this, &ATU_FPVDrone::InputYaw);

    PlayerInputComponent->BindAction(TEXT("FPVToggleMode"), IE_Pressed, this, &ATU_FPVDrone::ToggleFlightMode);
    PlayerInputComponent->BindAction(TEXT("FPVArm"), IE_Pressed, this, &ATU_FPVDrone::ToggleArmed);
    PlayerInputComponent->BindAction(TEXT("FPVReset"), IE_Pressed, this, &ATU_FPVDrone::ResetDrone);
    PlayerInputComponent->BindAction(TEXT("FPVExit"), IE_Pressed, this, &ATU_FPVDrone::ExitDrone);
}

void ATU_FPVDrone::InputThrottle(float Value)
{
    if (bExternalRCInputEnabled)
    {
        return;
    }

    // Spring-centered accessibility mapping. Native RC input bypasses this path.
    ThrottleInput = FMath::Clamp(0.5f + (Value * 0.5f), 0.0f, 1.0f);
}

void ATU_FPVDrone::InputRoll(float Value)
{
    if (!bExternalRCInputEnabled)
    {
        RollInput = FMath::Clamp(Value, -1.0f, 1.0f);
    }
}

void ATU_FPVDrone::InputPitch(float Value)
{
    if (!bExternalRCInputEnabled)
    {
        PitchInput = FMath::Clamp(Value, -1.0f, 1.0f);
    }
}

void ATU_FPVDrone::InputYaw(float Value)
{
    if (!bExternalRCInputEnabled)
    {
        YawInput = FMath::Clamp(Value, -1.0f, 1.0f);
    }
}

void ATU_FPVDrone::ToggleFlightMode()
{
    FlightMode = FlightMode == ETUFPVFlightMode::Acro
        ? ETUFPVFlightMode::Stabilized
        : ETUFPVFlightMode::Acro;

    ClearControllerState();
}

void ATU_FPVDrone::ToggleArmed()
{
    bArmed = !bArmed;
    bSignalFailsafeActive = false;
    SignalFailsafeElapsed = 0.0f;
    ClearControllerState();
}

void ATU_FPVDrone::ResetDrone()
{
    bArmed = false;
    bSignalFailsafeActive = false;
    SignalFailsafeElapsed = 0.0f;
    ClearControllerState();

    PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void ATU_FPVDrone::ServiceDrone()
{
    bArmed = false;
    ClearControllerState();

    if (Battery)
    {
        Battery->ResetBattery();
    }

    for (float& Health : MotorHealth)
    {
        Health = 1.0f;
    }
}

void ATU_FPVDrone::SetReturnPawn(APawn* PawnToReturnTo)
{
    ReturnPawn = PawnToReturnTo;

    if (Signal)
    {
        Signal->SetSignalOrigin(PawnToReturnTo);
    }
}

void ATU_FPVDrone::ExitDrone()
{
    if (!Controller || !IsValid(ReturnPawn))
    {
        return;
    }

    bArmed = false;
    ClearControllerState();

    AController* DroneController = Controller;
    DroneController->Possess(ReturnPawn);
}

void ATU_FPVDrone::SetExternalRCEnabled(bool bEnabled)
{
    bExternalRCInputEnabled = bEnabled;
}

void ATU_FPVDrone::SetExternalRCInput(float Throttle01, float Roll, float Pitch, float Yaw)
{
    if (!bExternalRCInputEnabled)
    {
        return;
    }

    ThrottleInput = FMath::Clamp(Throttle01, 0.0f, 1.0f);
    RollInput = FMath::Clamp(Roll, -1.0f, 1.0f);
    PitchInput = FMath::Clamp(Pitch, -1.0f, 1.0f);
    YawInput = FMath::Clamp(Yaw, -1.0f, 1.0f);
}

FVector4 ATU_FPVDrone::GetMotorCommands() const
{
    return FVector4(MotorCommands[0], MotorCommands[1], MotorCommands[2], MotorCommands[3]);
}

FVector4 ATU_FPVDrone::GetMotorHealth() const
{
    return FVector4(MotorHealth[0], MotorHealth[1], MotorHealth[2], MotorHealth[3]);
}

FFPVTelemetry ATU_FPVDrone::GetTelemetry() const
{
    FFPVTelemetry Telemetry;
    Telemetry.FlightMode = FlightMode;
    Telemetry.bArmed = bArmed;
    Telemetry.bSignalFailsafe = bSignalFailsafeActive;
    Telemetry.PropwashIntensity = CurrentPropwashIntensity;
    Telemetry.MotorHealth = GetMotorHealth();

    if (PhysicsBody)
    {
        Telemetry.SpeedMetersPerSecond = PhysicsBody->GetPhysicsLinearVelocity().Size() / CmPerMeter;
        Telemetry.AltitudeAGLMeters = ComputeAltitudeAGLMeters();
    }

    if (Battery)
    {
        Telemetry.BatteryVoltage = Battery->GetVoltage();
        Telemetry.BatteryPercent = Battery->GetBatteryPercent();
        Telemetry.CurrentDrawAmps = Battery->GetCurrentDrawAmps();
        Telemetry.bLowBattery = Battery->IsLowVoltage();
    }

    if (Signal)
    {
        Telemetry.VideoQuality = Signal->GetVideoQuality();
        Telemetry.ControlQuality = Signal->GetControlQuality();
        Telemetry.VideoLatencyMs = Signal->GetVideoLatencyMs();
        Telemetry.LinkDistanceMeters = Signal->GetDistanceMeters();
    }

    return Telemetry;
}

float ATU_FPVDrone::ApplyRateCurve(float Input) const
{
    const float Clamped = FMath::Clamp(Input, -1.0f, 1.0f);
    const float Magnitude = FMath::Abs(Clamped);
    const float Exponent = 1.0f + (FMath::Clamp(RateExpo, 0.0f, 1.0f) * 2.0f);
    return FMath::Sign(Clamped) * FMath::Pow(Magnitude, Exponent);
}

void ATU_FPVDrone::UpdateFlightController(float DeltaSeconds)
{
    const FVector WorldAngularVelocity = PhysicsBody->GetPhysicsAngularVelocityInRadians();
    const FVector LocalAngularVelocity = PhysicsBody->GetComponentTransform().InverseTransformVectorNoScale(WorldAngularVelocity);

    float TargetRollRate = FMath::DegreesToRadians(ApplyRateCurve(RollInput) * MaxRollRateDegPerSec);
    float TargetPitchRate = FMath::DegreesToRadians(ApplyRateCurve(PitchInput) * MaxPitchRateDegPerSec);
    const float TargetYawRate = FMath::DegreesToRadians(ApplyRateCurve(YawInput) * MaxYawRateDegPerSec);

    if (FlightMode == ETUFPVFlightMode::Stabilized)
    {
        const FRotator Rotation = PhysicsBody->GetComponentRotation();
        const float DesiredRoll = RollInput * MaxLevelAngleDeg;
        const float DesiredPitch = PitchInput * MaxLevelAngleDeg;

        const float RollAngleError = FMath::FindDeltaAngleDegrees(Rotation.Roll, DesiredRoll);
        const float PitchAngleError = FMath::FindDeltaAngleDegrees(Rotation.Pitch, DesiredPitch);

        const float MaxRollRateRad = FMath::DegreesToRadians(MaxRollRateDegPerSec);
        const float MaxPitchRateRad = FMath::DegreesToRadians(MaxPitchRateDegPerSec);

        TargetRollRate = FMath::Clamp(FMath::DegreesToRadians(RollAngleError * LevelRateGain), -MaxRollRateRad, MaxRollRateRad);
        TargetPitchRate = FMath::Clamp(FMath::DegreesToRadians(PitchAngleError * LevelRateGain), -MaxPitchRateRad, MaxPitchRateRad);
    }

    const float RollCorrection = RunPID(
        RollPID,
        TargetRollRate - LocalAngularVelocity.X,
        TargetRollRate,
        DeltaSeconds,
        RollIntegral,
        PreviousRollError,
        PreviousRollTarget);

    const float PitchCorrection = RunPID(
        PitchPID,
        TargetPitchRate - LocalAngularVelocity.Y,
        TargetPitchRate,
        DeltaSeconds,
        PitchIntegral,
        PreviousPitchError,
        PreviousPitchTarget);

    const float YawCorrection = RunPID(
        YawPID,
        TargetYawRate - LocalAngularVelocity.Z,
        TargetYawRate,
        DeltaSeconds,
        YawIntegral,
        PreviousYawError,
        PreviousYawTarget);

    // X-quad mixer. Motor order: front-left, front-right, rear-right, rear-left.
    DesiredMotorCommands[0] = ThrottleInput - RollCorrection - PitchCorrection + YawCorrection;
    DesiredMotorCommands[1] = ThrottleInput + RollCorrection - PitchCorrection - YawCorrection;
    DesiredMotorCommands[2] = ThrottleInput + RollCorrection + PitchCorrection + YawCorrection;
    DesiredMotorCommands[3] = ThrottleInput - RollCorrection + PitchCorrection - YawCorrection;

    for (int32 Index = 0; Index < 4; ++Index)
    {
        DesiredMotorCommands[Index] = FMath::Clamp(DesiredMotorCommands[Index], 0.0f, 1.0f);
    }
}

float ATU_FPVDrone::RunPID(
    const FFPVPIDGains& Gains,
    float Error,
    float TargetRate,
    float DeltaSeconds,
    float& Integral,
    float& PreviousError,
    float& PreviousTargetRate)
{
    Integral = FMath::Clamp(Integral + (Error * DeltaSeconds), -1.5f, 1.5f);

    const float SafeDelta = FMath::Max(DeltaSeconds, 0.001f);
    const float Derivative = (Error - PreviousError) / SafeDelta;
    const float FeedForward = (TargetRate - PreviousTargetRate) / SafeDelta;

    PreviousError = Error;
    PreviousTargetRate = TargetRate;

    const float Output =
        (Gains.P * Error) +
        (Gains.I * Integral) +
        (Gains.D * Derivative) +
        (Gains.FeedForward * FeedForward);

    return FMath::Clamp(Output, -0.45f, 0.45f);
}

void ATU_FPVDrone::ApplyRotorForces(float DeltaSeconds)
{
    const float Alpha = MotorResponseSeconds <= SMALL_NUMBER
        ? 1.0f
        : 1.0f - FMath::Exp(-DeltaSeconds / MotorResponseSeconds);

    const float GroundEffectMultiplier = ComputeGroundEffectMultiplier();
    const float PropwashMultiplier = 1.0f - (FMath::Clamp(CurrentPropwashIntensity, 0.0f, 1.0f) * PropwashMaxThrustLoss);
    const float BatteryThrustScale = Battery ? Battery->GetThrustScale() : 1.0f;
    const FVector Up = PhysicsBody->GetUpVector();

    const FVector LocalMotorPositions[4] =
    {
        FVector( ArmHalfSpanCm, -ArmHalfSpanCm, 0.0f),
        FVector( ArmHalfSpanCm,  ArmHalfSpanCm, 0.0f),
        FVector(-ArmHalfSpanCm,  ArmHalfSpanCm, 0.0f),
        FVector(-ArmHalfSpanCm, -ArmHalfSpanCm, 0.0f)
    };

    const float SpinSigns[4] = { 1.0f, -1.0f, 1.0f, -1.0f };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        MotorCommands[Index] = FMath::Lerp(MotorCommands[Index], DesiredMotorCommands[Index], Alpha);

        const float CommandSquared = MotorCommands[Index] * MotorCommands[Index];
        const float RotorHealth = FMath::Clamp(MotorHealth[Index], 0.0f, 1.0f);
        const float ThrustNewton =
            MaxMotorThrustNewton *
            CommandSquared *
            GroundEffectMultiplier *
            PropwashMultiplier *
            BatteryThrustScale *
            RotorHealth;

        const FVector Force = Up * (ThrustNewton * NewtonToUnrealForce);
        const FVector WorldMotorPosition = PhysicsBody->GetComponentTransform().TransformPosition(LocalMotorPositions[Index]);
        PhysicsBody->AddForceAtLocation(Force, WorldMotorPosition, NAME_None);

        const float ReactionTorqueNm = MaxReactionTorqueNm * CommandSquared * SpinSigns[Index] * RotorHealth;
        PhysicsBody->AddTorqueInRadians(
            Up * (ReactionTorqueNm * NewtonMeterToUnrealTorque),
            NAME_None,
            false);
    }

    ApplyPropwashDisturbance(CurrentPropwashIntensity);
    ApplyDamagedPropVibration();
}

void ATU_FPVDrone::ApplyAerodynamicDrag()
{
    const FVector WorldVelocityCm = PhysicsBody->GetPhysicsLinearVelocity();
    const FVector LocalVelocityM =
        PhysicsBody->GetComponentTransform().InverseTransformVectorNoScale(WorldVelocityCm) / CmPerMeter;

    const FVector LocalDragNewton(
        -0.5f * AirDensityKgM3 * DragCoefficient * ProjectedAreaM2.X * LocalVelocityM.X * FMath::Abs(LocalVelocityM.X),
        -0.5f * AirDensityKgM3 * DragCoefficient * ProjectedAreaM2.Y * LocalVelocityM.Y * FMath::Abs(LocalVelocityM.Y),
        -0.5f * AirDensityKgM3 * DragCoefficient * ProjectedAreaM2.Z * LocalVelocityM.Z * FMath::Abs(LocalVelocityM.Z));

    const FVector WorldDrag = PhysicsBody->GetComponentTransform().TransformVectorNoScale(LocalDragNewton) * NewtonToUnrealForce;
    PhysicsBody->AddForce(WorldDrag, NAME_None, false);
}

void ATU_FPVDrone::UpdateBattery(float DeltaSeconds)
{
    if (!Battery)
    {
        return;
    }

    float CurrentAmps = ElectronicsIdleCurrentAmps;
    if (bArmed)
    {
        for (float Command : MotorCommands)
        {
            CurrentAmps += (Command * Command) * MaxMotorCurrentAmps;
        }
    }

    Battery->ConsumeCurrent(CurrentAmps, DeltaSeconds);
}

void ATU_FPVDrone::UpdateSignalFailsafe(float DeltaSeconds)
{
    if (!bEnableSignalFailsafe || !Signal || !IsValid(ReturnPawn) || Signal->HasControlLink())
    {
        bSignalFailsafeActive = false;
        SignalFailsafeElapsed = 0.0f;
        return;
    }

    bSignalFailsafeActive = true;
    SignalFailsafeElapsed += DeltaSeconds;

    if (bArmed && SignalFailsafeElapsed >= SignalFailsafeDisarmSeconds)
    {
        bArmed = false;
        ClearControllerState();
    }
}

float ATU_FPVDrone::ComputePropwashIntensity() const
{
    if (!PhysicsBody || PropwashFullDescentMps <= PropwashStartDescentMps)
    {
        return 0.0f;
    }

    const FVector VelocityMps = PhysicsBody->GetPhysicsLinearVelocity() / CmPerMeter;
    const FVector Up = PhysicsBody->GetUpVector();
    const float VerticalVelocity = FVector::DotProduct(VelocityMps, Up);
    const float DescentSpeed = FMath::Max(0.0f, -VerticalVelocity);

    if (DescentSpeed <= PropwashStartDescentMps)
    {
        return 0.0f;
    }

    const float DescentFactor = FMath::Clamp(
        (DescentSpeed - PropwashStartDescentMps) /
        (PropwashFullDescentMps - PropwashStartDescentMps),
        0.0f,
        1.0f);

    const FVector LateralVelocity = VelocityMps - (Up * VerticalVelocity);
    const float LateralRatio = LateralVelocity.Size() / FMath::Max(PropwashLateralEscapeMps, 0.1f);
    const float WakeRetention = 1.0f / (1.0f + (LateralRatio * LateralRatio));

    return DescentFactor * WakeRetention;
}

void ATU_FPVDrone::ApplyPropwashDisturbance(float PropwashIntensity)
{
    if (PropwashIntensity <= SMALL_NUMBER || PropwashDisturbanceTorqueNm <= 0.0f || !GetWorld())
    {
        return;
    }

    float AverageCommand = 0.0f;
    for (float Command : MotorCommands)
    {
        AverageCommand += Command;
    }
    AverageCommand *= 0.25f;

    const float Time = GetWorld()->GetTimeSeconds();
    const float TorqueNm = PropwashDisturbanceTorqueNm * PropwashIntensity * AverageCommand;
    const FVector LocalTorque(
        FMath::Sin(Time * 61.0f) * TorqueNm,
        FMath::Sin((Time * 73.0f) + 1.7f) * TorqueNm,
        0.0f);

    const FVector WorldTorque = PhysicsBody->GetComponentTransform().TransformVectorNoScale(LocalTorque);
    PhysicsBody->AddTorqueInRadians(WorldTorque * NewtonMeterToUnrealTorque, NAME_None, false);
}

void ATU_FPVDrone::ApplyDamagedPropVibration()
{
    if (!GetWorld() || DamagedPropVibrationTorqueNm <= 0.0f)
    {
        return;
    }

    float DamageLoad = 0.0f;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        DamageLoad += (1.0f - FMath::Clamp(MotorHealth[Index], 0.0f, 1.0f)) * MotorCommands[Index] * MotorCommands[Index];
    }

    if (DamageLoad <= SMALL_NUMBER)
    {
        return;
    }

    const float Time = GetWorld()->GetTimeSeconds();
    const float TorqueNm = DamagedPropVibrationTorqueNm * DamageLoad;
    const FVector LocalTorque(
        FMath::Sin(Time * 109.0f) * TorqueNm,
        FMath::Cos(Time * 97.0f) * TorqueNm,
        FMath::Sin((Time * 83.0f) + 0.9f) * TorqueNm * 0.35f);

    const FVector WorldTorque = PhysicsBody->GetComponentTransform().TransformVectorNoScale(LocalTorque);
    PhysicsBody->AddTorqueInRadians(WorldTorque * NewtonMeterToUnrealTorque, NAME_None, false);
}

float ATU_FPVDrone::ComputeGroundEffectMultiplier() const
{
    if (!GetWorld() || GroundEffectRangeCm <= SMALL_NUMBER || GroundEffectStrength <= 0.0f)
    {
        return 1.0f;
    }

    const FVector Start = PhysicsBody->GetComponentLocation();
    const FVector End = Start - (PhysicsBody->GetUpVector() * GroundEffectRangeCm);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FPVGroundEffect), false, this);

    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return 1.0f;
    }

    const float NormalizedDistance = FMath::Clamp(Hit.Distance / GroundEffectRangeCm, 0.0f, 1.0f);
    const float Proximity = 1.0f - NormalizedDistance;
    return 1.0f + (GroundEffectStrength * Proximity * Proximity);
}

float ATU_FPVDrone::ComputeAltitudeAGLMeters() const
{
    if (!GetWorld() || !PhysicsBody)
    {
        return -1.0f;
    }

    const FVector Start = PhysicsBody->GetComponentLocation();
    const FVector End = Start - FVector(0.0f, 0.0f, 20000.0f);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FPVAltitudeAGL), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return Hit.Distance / CmPerMeter;
    }

    return -1.0f;
}

void ATU_FPVDrone::HandlePhysicsHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (NormalImpulse.IsNearlyZero() || MassKg <= SMALL_NUMBER || DamageFullDeltaVMps <= DamageStartDeltaVMps)
    {
        return;
    }

    // Chaos impulse is in kg*cm/s. Dividing by mass and 100 approximates delta-v in m/s.
    const float ImpactDeltaVMps = NormalImpulse.Size() / (MassKg * CmPerMeter);
    if (ImpactDeltaVMps <= DamageStartDeltaVMps)
    {
        return;
    }

    const float Severity = FMath::Clamp(
        (ImpactDeltaVMps - DamageStartDeltaVMps) /
        (DamageFullDeltaVMps - DamageStartDeltaVMps),
        0.0f,
        1.0f);

    const FVector LocalImpact = PhysicsBody->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
    const FVector LocalMotorPositions[4] =
    {
        FVector( ArmHalfSpanCm, -ArmHalfSpanCm, 0.0f),
        FVector( ArmHalfSpanCm,  ArmHalfSpanCm, 0.0f),
        FVector(-ArmHalfSpanCm,  ArmHalfSpanCm, 0.0f),
        FVector(-ArmHalfSpanCm, -ArmHalfSpanCm, 0.0f)
    };

    int32 NearestMotor = 0;
    float NearestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float DistanceSquared = (LocalImpact - LocalMotorPositions[Index]).SizeSquared2D();
        if (DistanceSquared < NearestDistanceSquared)
        {
            NearestDistanceSquared = DistanceSquared;
            NearestMotor = Index;
        }
    }

    MotorHealth[NearestMotor] = FMath::Clamp(
        MotorHealth[NearestMotor] - (Severity * MaxMotorDamagePerImpact),
        0.0f,
        1.0f);
}

void ATU_FPVDrone::ClearControllerState()
{
    RollIntegral = 0.0f;
    PitchIntegral = 0.0f;
    YawIntegral = 0.0f;
    PreviousRollError = 0.0f;
    PreviousPitchError = 0.0f;
    PreviousYawError = 0.0f;
    PreviousRollTarget = 0.0f;
    PreviousPitchTarget = 0.0f;
    PreviousYawTarget = 0.0f;

    for (int32 Index = 0; Index < 4; ++Index)
    {
        DesiredMotorCommands[Index] = 0.0f;
        if (!bArmed)
        {
            MotorCommands[Index] = 0.0f;
        }
    }
}
