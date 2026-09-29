#include "TU_OperatorCharacter.h"
#include "TUBetaUserSettings.h"

#include "Camera/CameraComponent.h"
#include "TUHandlingCameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Net/UnrealNetwork.h"
#include "TUWeaponPresentationComponent.h"
#include "TUHandlingSight.h"
#include "TUWeaponClearance.h"
#include "TUWeaponPartsComponent.h"
#include "TUWeaponContactProfile.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

ATU_OperatorCharacter::ATU_OperatorCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    FirstPersonCamera = CreateDefaultSubobject<UTUHandlingCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->bUsePawnControlRotation = true;
    // Ahead of the template neck/shoulders, still inside the movement capsule.
    FirstPersonCamera->SetRelativeLocation(FVector(18.f, 0.f, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - 14.f));

    FirstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmsMesh"));
    // Owner body and observer body share the same asset/pose in world space.
    // A hidden owner head avoids putting the camera inside visible face geometry.
    FirstPersonArmsMesh->SetupAttachment(GetCapsuleComponent());
    FirstPersonArmsMesh->SetOnlyOwnerSee(true);
    FirstPersonArmsMesh->bCastDynamicShadow = false;
    FirstPersonArmsMesh->CastShadow = false;
    FirstPersonArmsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->bCastHiddenShadow = true;
    GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
    FirstPersonArmsMesh->SetRelativeTransform(GetMesh()->GetRelativeTransform());
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Body(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
    if (Body.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(Body.Object);
        FirstPersonArmsMesh->SetSkeletalMesh(Body.Object);
    }
    PrototypeIdle = Idle.Object;
    WorldWeaponAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("WorldWeaponAnchor"));
    WorldWeaponAnchor->SetupAttachment(GetCapsuleComponent());
    WorldWeaponAnchor->SetRelativeLocation(FVector(20.f, 12.f, 46.f));
    WeaponPresentation = CreateDefaultSubobject<UTUWeaponPresentationComponent>(TEXT("WeaponPresentation"));

    bUseControllerRotationYaw = true;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = WalkSpeed;
        Movement->MaxWalkSpeedCrouched = CrouchSpeed;
        // Match a bent-knee crouch rather than collapsing a standing mannequin to 40cm.
        Movement->SetCrouchedHalfHeight(64.f);
        Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
    }
}

void ATU_OperatorCharacter::BeginPlay()
{
    Super::BeginPlay();
    FirstPersonArmsMesh->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
}

void ATU_OperatorCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATU_OperatorCharacter, ReplicatedPosture);
    DOREPLIFETIME(ATU_OperatorCharacter, ReadyPosture);
}

void ATU_OperatorCharacter::UpdatePosture()
{
    const uint8 Flags = (bIsADS ? 1 : 0) | (bIsSprinting ? 2 : 0) | (bIsLeaningLeft ? 4 : 0) | (bIsLeaningRight ? 8 : 0) | (ReadyPosture << 4);
    if (HasAuthority()) ReplicatedPosture = Flags;
    else ServerSetPosture(Flags);
}

void ATU_OperatorCharacter::ServerSetPosture_Implementation(uint8 Flags)
{
    ReplicatedPosture = Flags & 63;
    ReadyPosture = FMath::Min<uint8>((Flags >> 4) & 3, 2);
    bIsADS = (Flags & 1) != 0;
    bIsSprinting = (Flags & 2) != 0 && !bIsADS;
    bIsLeaningLeft = (Flags & 4) != 0;
    bIsLeaningRight = (Flags & 8) != 0 && !bIsLeaningLeft;
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::CycleReadyPosture()
{
    ReadyPosture = (ReadyPosture + 1) % 3;
    bIsADS = false;
    UpdateMovementSpeed();
    UpdatePosture();
}

void ATU_OperatorCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const uint8 Flags = IsLocallyControlled()
        ? ((bIsADS ? 1 : 0) | (bIsSprinting ? 2 : 0) | (bIsLeaningLeft ? 4 : 0) | (bIsLeaningRight ? 8 : 0))
        : ReplicatedPosture;
    const float TargetLean = (Flags & 4) ? -1.f : ((Flags & 8) ? 1.f : 0.f);
    ATU_WeaponBase* Weapon = WeaponPresentation ? WeaponPresentation->GetActiveWeapon() : nullptr;
    const bool bManipulating = Weapon && Weapon->GetActionState().bActive;
    const bool bLowered = (Flags & 2) != 0;
    const bool bAimed = (Flags & 1) != 0 && !bLowered && !bManipulating && ReadyPosture == 0 && CanAimWeapon();
    const float Blend = 1.f - FMath::Exp(-14.f * FMath::Max(0.f, DeltaSeconds));
    HandlingLean = FMath::Lerp(HandlingLean, TargetLean, Blend);
    HandlingADSAlpha = FMath::Lerp(HandlingADSAlpha, bAimed ? 1.f : 0.f, Blend);
    HandlingManipulationAlpha = FMath::Lerp(HandlingManipulationAlpha, bManipulating ? 1.f : 0.f, Blend);
    const FTransform EyeWorld = GetHandlingEyeWorld();
    const float ReadyPitch = ReadyPosture == 2 ? 45.f : ((bLowered || ReadyPosture == 1) ? -30.f : 0.f);
    HandlingReadyPitch = FMath::Lerp(HandlingReadyPitch, ReadyPitch, Blend);
    const FName WeaponId = Weapon ? Weapon->GetWeaponDefinition().WeaponId : NAME_None;
    const FTUWeaponMountProfile Mount = TUWeaponContact::ResolveMountProfile(WeaponId);
    // Shoulder placement is translational first and rotational second. Keep the
    // bore near the camera forward axis and bias the receiver toward the firing
    // side instead of yawing the complete weapon across the operator's centerline.
    const FRotator HipRotation(
        HandlingReadyPitch + Mount.HipRotation.Pitch,
        Mount.HipRotation.Yaw,
        Mount.HipRotation.Roll);
    const FTransform HipWorld = FTransform(HipRotation, Mount.HipOffset) * EyeWorld;
    FTransform WeaponWorld = HipWorld;
    FTUHandlingSightProfile Sight;
    if (Weapon && (HasAuthority() || IsLocallyControlled()) && Weapon->IsAiming() != bAimed)
        Weapon->SetAiming(bAimed);
    if (Weapon && TUHandlingSight::ResolveProfile(Weapon->GetWeaponBodyMesh(), Sight))
        WeaponWorld.Blend(HipWorld, TUHandlingSight::SolveWeaponWorld(EyeWorld, Sight), HandlingADSAlpha);
    // A real world manipulation pose keeps the receiver and firing grip in view.
    // Reload is not low-ready: preserve reach and view of the upper-chest workspace.
    // Measured rifle vertices keep the stock ahead of the near plane in this
    // diagonal workspace; the previous near-axis pose intersected the near plane.
    // The weapon stays mounted. Reload only opens a small workspace angle while
    // the support-side IK and weapon-part animation perform the magazine/control
    // manipulation. This prevents the whole rifle from corkscrewing during reload.
    const FRotator ReloadRotation(
        HandlingReadyPitch + Mount.ReloadRotation.Pitch,
        Mount.ReloadRotation.Yaw,
        Mount.ReloadRotation.Roll);
    const FTransform ManipulationWorld = FTransform(ReloadRotation, Mount.ReloadOffset) * EyeWorld;
    FTransform PresentedWorld;
    PresentedWorld.Blend(WeaponWorld, ManipulationWorld, HandlingManipulationAlpha);
    bWeaponClearanceBlocked=false;
    if(Weapon && Weapon->GetWeaponBodyMesh()) {
        FVector BMin,BMax;Weapon->GetWeaponBodyMesh()->GetLocalBounds(BMin,BMax);
        FBox Bounds=FBox(BMin,BMax).TransformBy(Weapon->GetWeaponBodyMesh()->GetRelativeTransform());
        if(bManipulating && Weapon->PartsPresentation && Weapon->PartsPresentation->IsSupported())
            Bounds+=FBox(FVector(1.f,-9.f,-35.f),FVector(22.f,9.f,3.f));
        const auto Clearance=TUWeaponClearance::Resolve(GetWorld(),this,Weapon,PresentedWorld,EyeWorld,Bounds,PreviousClearWeaponPose);
        bWeaponClearanceBlocked=Clearance.bDesiredBlocked;
        // Do not solve an impossible reload by folding the operator/weapon outside
        // the player's view. Preserve the readable manipulation pose and let the
        // authoritative action system refuse/interrupt manipulation when clearance
        // is insufficient; clearance still blocks firing independently.
        if(bManipulating && Clearance.bDesiredBlocked) {
            PresentedWorld=ManipulationWorld;
            Weapon->StopFire();
        } else if(Clearance.bResolved) {
            PresentedWorld=Clearance.Pose;PreviousClearWeaponPose=PresentedWorld;
        } else {
            Weapon->StopFire();GetCharacterMovement()->StopMovementImmediately();
        }
    }
    // Procedural firing/movement presentation is cosmetic and deliberately added
    // after obstruction resolution. It never changes muzzle authority, hit traces,
    // action state or the clearance decision. Manipulation owns the workspace while
    // a reload is active, so locomotion/recoil offsets do not fight the hands.
    if (WeaponPresentation && !bManipulating)
        PresentedWorld = WeaponPresentation->GetWeaponPresentationOffset() * PresentedWorld;
    WorldWeaponAnchor->SetWorldTransform(PresentedWorld);
    // The evaluated upper body handles lean; rotating the whole mesh tilts the feet.
    const FRotator BodyRotation(0.f, -90.f, 0.f);
    GetMesh()->SetRelativeRotation(BodyRotation);
    FirstPersonArmsMesh->SetRelativeRotation(BodyRotation);
    FirstPersonArmsMesh->SetRelativeLocation(GetMesh()->GetRelativeLocation());
    if (IsLocallyControlled())
    {
        FirstPersonCamera->SetRelativeLocation(FVector(18.f, HandlingLean * 8.f, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - 14.f));
        // Camera-only effects deliberately never feed world aiming or hit traces.
        if(const auto* S=UTUBetaUserSettings::Get()) FirstPersonCamera->SetFieldOfView(S->FieldOfView);
        FirstPersonCamera->ClearAdditiveOffset();
        const FRotator CameraRecoil = WeaponPresentation ? WeaponPresentation->GetCameraRecoilRotation() : FRotator::ZeroRotator;
        const float Sway = WeaponPresentation && (!UTUBetaUserSettings::Get() || UTUBetaUserSettings::Get()->bCameraSwayEnabled) ? WeaponPresentation->GetSwayOffset() : 0.f;
        // Camera recoil is its own bounded channel. Lean remains the existing base
        // roll and world-space aiming remains untouched.
        const FRotator CameraPresentation(
            CameraRecoil.Pitch,
            CameraRecoil.Yaw,
            HandlingLean * 8.f + Sway + CameraRecoil.Roll);
        // Add zoom without overwriting the user's configured base FOV.
        FirstPersonCamera->AddAdditiveOffset(FTransform(CameraPresentation), -15.f * HandlingADSAlpha);
    }
}

FTransform ATU_OperatorCharacter::GetHandlingEyeWorld() const
{
    FRotator Aim = GetBaseAimRotation();
    Aim.Roll = HandlingLean * 8.f;
    const FVector Eye = GetActorTransform().TransformPosition(FVector(18.f, HandlingLean * 8.f,
        GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - 14.f));
    return FTransform(Aim, Eye);
}

void ATU_OperatorCharacter::ApplyHandlingCapturePose(bool bADS, bool bCrouched, float Lean, float PitchDegrees)
{
    FString CaptureId;
    if (!FParse::Value(FCommandLine::Get(), TEXT("TUHandlingCapture="), CaptureId)) return;
    if (bADS) StartADS(); else StopADS();
    if (bCrouched) StartCrouch(); else StopCrouch();
    bIsLeaningLeft = Lean < 0.f;
    bIsLeaningRight = Lean > 0.f;
    if (Controller)
    {
        FRotator Aim = Controller->GetControlRotation();
        Aim.Pitch = FMath::Clamp(PitchDegrees, -80.f, 80.f);
        Controller->SetControlRotation(Aim);
    }
    UpdatePosture();
}

void ATU_OperatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATU_OperatorCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATU_OperatorCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ATU_OperatorCharacter::LookUp);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ATU_OperatorCharacter::Turn);

    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ATU_OperatorCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ATU_OperatorCharacter::StopSprint);

    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ATU_OperatorCharacter::StartCrouch);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &ATU_OperatorCharacter::StopCrouch);

    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Pressed, this, &ATU_OperatorCharacter::StartADS);
    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Released, this, &ATU_OperatorCharacter::StopADS);

    PlayerInputComponent->BindAction(TEXT("LeanLeft"), IE_Pressed, this, &ATU_OperatorCharacter::StartLeanLeft);
    PlayerInputComponent->BindAction(TEXT("LeanLeft"), IE_Released, this, &ATU_OperatorCharacter::StopLeanLeft);

    PlayerInputComponent->BindAction(TEXT("LeanRight"), IE_Pressed, this, &ATU_OperatorCharacter::StartLeanRight);
    PlayerInputComponent->BindAction(TEXT("LeanRight"), IE_Released, this, &ATU_OperatorCharacter::StopLeanRight);

    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ATU_OperatorCharacter::Interact);
    PlayerInputComponent->BindAction(TEXT("CycleReady"), IE_Pressed, this, &ATU_OperatorCharacter::CycleReadyPosture);
}

void ATU_OperatorCharacter::MoveForward(float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void ATU_OperatorCharacter::MoveRight(float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void ATU_OperatorCharacter::LookUp(float Value)
{
    const auto* S=UTUBetaUserSettings::Get();
    AddControllerPitchInput(Value*(S?S->MouseSensitivity:1.f)*(S && S->bInvertVertical?-1.f:1.f));
}

void ATU_OperatorCharacter::Turn(float Value)
{
    const auto* S=UTUBetaUserSettings::Get();
    AddControllerYawInput(Value*(S?S->MouseSensitivity:1.f));
}

void ATU_OperatorCharacter::StartSprint()
{
    bIsSprinting = !bIsADS;
    UpdateMovementSpeed();
    UpdatePosture();
}

void ATU_OperatorCharacter::StopSprint()
{
    bIsSprinting = false;
    UpdateMovementSpeed();
    UpdatePosture();
}

void ATU_OperatorCharacter::StartCrouch()
{
    Crouch();
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StopCrouch()
{
    UnCrouch();
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StartADS()
{
    if (!CanAimWeapon()) return;
    bIsADS = true;
    ReadyPosture = 0;
    bIsSprinting = false;
    if (WeaponPresentation && WeaponPresentation->GetActiveWeapon())
        WeaponPresentation->GetActiveWeapon()->SetAiming(!WeaponPresentation->bActionInProgress);
    UpdateMovementSpeed();
    UpdatePosture();
}

void ATU_OperatorCharacter::StopADS()
{
    bIsADS = false;
    if (WeaponPresentation && WeaponPresentation->GetActiveWeapon())
        WeaponPresentation->GetActiveWeapon()->SetAiming(false);
    UpdateMovementSpeed();
    UpdatePosture();
}

void ATU_OperatorCharacter::StartLeanLeft()
{
    bIsLeaningLeft = true;
    bIsLeaningRight = false;
    UpdatePosture();
}

void ATU_OperatorCharacter::StopLeanLeft()
{
    bIsLeaningLeft = false;
    UpdatePosture();
}

void ATU_OperatorCharacter::StartLeanRight()
{
    bIsLeaningRight = true;
    bIsLeaningLeft = false;
    UpdatePosture();
}

void ATU_OperatorCharacter::StopLeanRight()
{
    bIsLeaningRight = false;
    UpdatePosture();
}

void ATU_OperatorCharacter::Interact()
{
    // Placeholder: interaction logic implemented in later phases.
}

void ATU_OperatorCharacter::UpdateMovementSpeed()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        float TargetSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;

        if (bIsCrouched)
        {
            TargetSpeed = CrouchSpeed;
        }

        if (bIsADS)
        {
            TargetSpeed *= ADSMovementMultiplier;
        }

        Movement->MaxWalkSpeed = TargetSpeed;
        Movement->MaxWalkSpeedCrouched = CrouchSpeed * ADSMovementMultiplier;
    }
}

void ATU_OperatorCharacter::SuspendInputForMenu()
{
    StopSprint();StopADS();StopLeanLeft();StopLeanRight();
    if(WeaponPresentation && WeaponPresentation->GetActiveWeapon()) WeaponPresentation->GetActiveWeapon()->StopFire();
}
