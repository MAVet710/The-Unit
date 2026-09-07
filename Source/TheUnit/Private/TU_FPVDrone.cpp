#include "TU_FPVDrone.h"

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

    FPVCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPVCamera"));
    FPVCamera->SetupAttachment(PhysicsBody);
    FPVCamera->SetRelativeLocation(FVector(11.0f, 0.0f, 2.0f));

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

    if (bArmed)
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

    ApplyRotorForces(DeltaSeconds);
    ApplyAerodynamicDrag();
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
    // Standard gamepads are spring-centered, while an RC throttle is not.
    // Mapping [-1, 1] -> [0, 1] gives keyboard/gamepad testing a usable hover midpoint.
    ThrottleInput = FMath::Clamp(0.5f + (Value * 0.5f), 0.0f, 1.0f);
}

void ATU_FPVDrone::InputRoll(float Value)
{
    RollInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void ATU_FPVDrone::InputPitch(float Value)
{
    PitchInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void ATU_FPVDrone::InputYaw(float Value)
{
    YawInput = FMath::Clamp(Value, -1.0f, 1.0f);
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
    ClearControllerState();
}

void ATU_FPVDrone::ResetDrone()
{
    bArmed = false;
    ClearControllerState();

    PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void ATU_FPVDrone::SetReturnPawn(APawn* PawnToReturnTo)
{
    ReturnPawn = PawnToReturnTo;
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

FVector4 ATU_FPVDrone::GetMotorCommands() const
{
    return FVector4(MotorCommands[0], MotorCommands[1], MotorCommands[2], MotorCommands[3]);
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

    const float Derivative = (Error - PreviousError) / FMath::Max(DeltaSeconds, 0.001f);
    const float FeedForward = (TargetRate - PreviousTargetRate) / FMath::Max(DeltaSeconds, 0.001f);

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
    const FVector Up = PhysicsBody->GetUpVector();

    const FVector LocalMotorPositions[4] =
    {
        FVector( ArmHalfSpanCm, -ArmHalfSpanCm, 0.0f), // front-left
        FVector( ArmHalfSpanCm,  ArmHalfSpanCm, 0.0f), // front-right
        FVector(-ArmHalfSpanCm,  ArmHalfSpanCm, 0.0f), // rear-right
        FVector(-ArmHalfSpanCm, -ArmHalfSpanCm, 0.0f)  // rear-left
    };

    const float SpinSigns[4] = { 1.0f, -1.0f, 1.0f, -1.0f };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        MotorCommands[Index] = FMath::Lerp(MotorCommands[Index], DesiredMotorCommands[Index], Alpha);

        // Rotor thrust is approximately proportional to RPM squared. The motor
        // command is treated as normalized RPM for this first flight model.
        const float CommandSquared = MotorCommands[Index] * MotorCommands[Index];
        const float ThrustNewton = MaxMotorThrustNewton * CommandSquared * GroundEffectMultiplier;
        const FVector Force = Up * (ThrustNewton * NewtonToUnrealForce);
        const FVector WorldMotorPosition = PhysicsBody->GetComponentTransform().TransformPosition(LocalMotorPositions[Index]);

        PhysicsBody->AddForceAtLocation(Force, WorldMotorPosition, NAME_None);

        const float ReactionTorqueNm = MaxReactionTorqueNm * CommandSquared * SpinSigns[Index];
        PhysicsBody->AddTorqueInRadians(
            Up * (ReactionTorqueNm * NewtonMeterToUnrealTorque),
            NAME_None,
            false);
    }
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
