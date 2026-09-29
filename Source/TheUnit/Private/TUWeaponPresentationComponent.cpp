#include "TUWeaponPresentationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "TU_OperatorCharacter.h"
#include "Animation/AnimSequence.h"
#include "TUHandlingAnimInstance.h"
#include "TUWeaponPartsComponent.h"
#include "TUWeaponContactProfile.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
UTUWeaponPresentationComponent::UTUWeaponPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Rifle(TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle"));
    PrototypeRifle = Rifle.Object;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Reload(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
    PrototypeReload = Reload.Object;
    PrototypeIdle = Idle.Object;
}
void UTUWeaponPresentationComponent::InitializeForWeapon(ATU_WeaponBase* Weapon)
{
    if (ActiveWeapon == Weapon) return;
    if (IsValid(ActiveWeapon)) ActiveWeapon->OnShotFired.RemoveDynamic(this, &UTUWeaponPresentationComponent::HandleShot);
    ActiveWeapon = Weapon;
    LastAction.Invalidate(); LastRevision = INDEX_NONE; bActionInProgress = false;
    WeaponRecoilLocation = FVector::ZeroVector;
    WeaponRecoilRotation = FRotator::ZeroRotator;
    CameraRecoilRotation = FRotator::ZeroRotator;
    WeaponSwayLocation = FVector::ZeroVector;
    WeaponSwayRotation = FRotator::ZeroRotator;
    ShotSequence = 0;
    if (!IsValid(Weapon)) { bCalibratedContacts = false; return; }
    AddTickPrerequisiteActor(GetOwner());
    if (auto* Operator = Cast<ATU_OperatorCharacter>(GetOwner()))
    {
        for (USkeletalMeshComponent* Body : {Operator->GetMesh(), Operator->GetOwnerBodyMesh()})
        {
            if (!Body) continue;
            Body->SetAnimInstanceClass(UTUHandlingAnimInstance::StaticClass());
            Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            Body->PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
            Body->AddTickPrerequisiteComponent(this);
            if (Body == Operator->GetOwnerBodyMesh()) Body->HideBoneByName(TEXT("neck_01"), EPhysBodyOp::PBO_None);
        }
    }
    Weapon->OnShotFired.AddUniqueDynamic(this, &UTUWeaponPresentationComponent::HandleShot);
    if (UStaticMeshComponent* Mesh = Weapon->GetWeaponBodyMesh())
    {
        // Only rifle definitions use this asset; pistols must not masquerade as rifles.
        const FString WeaponId = Weapon->GetWeaponDefinition().WeaponId.ToString();
        if (PrototypeRifle && (WeaponId.Contains(TEXT("Rifle")) || WeaponId.Contains(TEXT("Carbine")) || WeaponId.Contains(TEXT("M110"))))
        {
            Mesh->SetStaticMesh(PrototypeRifle);
            // Epic's template rifle is authored with its barrel along +Y. The
            // canonical actor aims along +X; rotate only the mesh, not hit authority.
            Mesh->SetRelativeTransform(FTransform(FRotator(0.f, -90.f, 0.f)));
        }
        Mesh->SetOnlyOwnerSee(false);
        Mesh->SetOwnerNoSee(false);
        Mesh->SetCastShadow(true);
    }
}
void UTUWeaponPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    InitializeForWeapon(nullptr);
    Super::EndPlay(Reason);
}
float UTUWeaponPresentationComponent::DecayOffset(float Offset, float Rate, float DeltaTime)
{
    return Offset * FMath::Exp(-FMath::Max(0.f, Rate) * FMath::Max(0.f, DeltaTime));
}
FVector UTUWeaponPresentationComponent::DecayVector(const FVector& Offset, float Rate, float DeltaTime)
{
    return Offset * FMath::Exp(-FMath::Max(0.f, Rate) * FMath::Max(0.f, DeltaTime));
}
FRotator UTUWeaponPresentationComponent::DecayRotation(const FRotator& Offset, float Rate, float DeltaTime)
{
    const float Scale = FMath::Exp(-FMath::Max(0.f, Rate) * FMath::Max(0.f, DeltaTime));
    return FRotator(Offset.Pitch * Scale, Offset.Yaw * Scale, Offset.Roll * Scale);
}
FTransform UTUWeaponPresentationComponent::GetWeaponPresentationOffset() const
{
    const FVector Location = WeaponRecoilLocation + WeaponSwayLocation;
    const FRotator Rotation(
        WeaponRecoilRotation.Pitch + WeaponSwayRotation.Pitch,
        WeaponRecoilRotation.Yaw + WeaponSwayRotation.Yaw,
        WeaponRecoilRotation.Roll + WeaponSwayRotation.Roll);
    return FTransform(Rotation, Location);
}
void UTUWeaponPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    const FTUWeaponPresentationProfile Profile = TUWeaponContact::ResolvePresentationProfile(
        IsValid(ActiveWeapon) ? ActiveWeapon->GetWeaponDefinition().WeaponId : NAME_None);
    WeaponRecoilLocation = DecayVector(WeaponRecoilLocation, Profile.WeaponRecoveryRate, DeltaTime);
    WeaponRecoilRotation = DecayRotation(WeaponRecoilRotation, Profile.WeaponRecoveryRate, DeltaTime);
    CameraRecoilRotation = DecayRotation(CameraRecoilRotation, Profile.CameraRecoveryRate, DeltaTime);
    MotionTime += FMath::Max(0.f, DeltaTime);
    const float Speed = GetOwner() ? GetOwner()->GetVelocity().Size2D() : 0.f;
    const float MoveAlpha = FMath::Clamp(Speed / 550.f, 0.f, 1.f);
    const float ADSScale = IsValid(ActiveWeapon) && ActiveWeapon->IsAiming() ? 0.35f : 1.f;
    const double Phase = MotionTime * UE_TWO_PI * Profile.SwayFrequencyHz;
    const float Side = FMath::Sin(Phase) * Profile.SwayLocationCm * MoveAlpha * ADSScale;
    const float Lift = FMath::Cos(Phase * 2.0) * Profile.SwayLocationCm * 0.45f * MoveAlpha * ADSScale;
    WeaponSwayLocation = FVector(0.f, Side, Lift);
    WeaponSwayRotation = FRotator(
        FMath::Cos(Phase) * Profile.SwayRotationDegrees * 0.35f,
        FMath::Sin(Phase) * Profile.SwayRotationDegrees * 0.45f,
        FMath::Sin(Phase) * Profile.SwayRotationDegrees);
    SwayOffset = FMath::Sin(Phase) * MoveAlpha * 0.3f;
    if (IsValid(ActiveWeapon)) PresentPhase(ActiveWeapon->GetActionState());
    if (IsValid(ActiveWeapon) && ActiveWeapon->PartsPresentation) ActiveWeapon->PartsPresentation->UpdatePresentation(DeltaTime);
    UpdateEvaluatedAnimation(DeltaTime);
}
void UTUWeaponPresentationComponent::PresentPhase(const FTUWeaponActionState& State)
{
    if (State.ActionId == LastAction && State.Revision <= LastRevision) return;
    const bool bInitialSnapshot = LastRevision == INDEX_NONE;
    if (State.ActionId != LastAction && State.bActive && PrototypeReload)
    {
        // A resumed action has a new ID but starts at its already committed phase.
        EvaluatedAnimationTime = PhaseAnimationFraction(State.Phase, 1.f, 1.f) * PrototypeReload->GetPlayLength();
    }
    LastAction = State.ActionId; LastRevision = State.Revision; PresentedPhase = State.Phase;
    bActionInProgress = State.bActive;
    if (bInitialSnapshot || !IsValid(ActiveWeapon)) return;
    USoundBase* Sound = nullptr;
    switch (State.Phase)
    {
        case ETUWeaponActionPhase::Removed: Sound = MagazineReleaseSound; break;
        case ETUWeaponActionPhase::Acquired: Sound = MagazineAcquireSound; break;
        case ETUWeaponActionPhase::Inserted: Sound = MagazineInsertSound; break;
        case ETUWeaponActionPhase::Chambered: Sound = ActionCycleSound; break;
        default: break;
    }
    if (Sound) UGameplayStatics::PlaySoundAtLocation(this, Sound, ActiveWeapon->GetActorLocation());
}
void UTUWeaponPresentationComponent::HandleShot(FTUWeaponShotResult Result)
{
    if (!Result.bFired || !IsValid(ActiveWeapon)) return;

    const FWeaponDefinition Definition = ActiveWeapon->GetWeaponDefinition();
    const FTUWeaponPresentationProfile Profile = TUWeaponContact::ResolvePresentationProfile(Definition.WeaponId);
    const bool bAutomatic = ActiveWeapon->GetCurrentFireMode() != ETUFireMode::SemiAuto;
    const float ModeScale = bAutomatic ? Profile.AutoImpulseScale : 1.f;

    // Deterministic lateral sequence avoids noisy camera RNG while preventing
    // every shot from looking like the exact same vertical keyframe.
    ++ShotSequence;
    const float Pattern = FMath::Sin(static_cast<float>(ShotSequence) * 2.39996323f);
    const float Pitch = FMath::Max(0.f, Definition.RecoilPitch) * ModeScale;
    const float Yaw = Definition.RecoilYaw * Pattern * ModeScale;

    WeaponRecoilLocation += FVector(
        -Pitch * Profile.WeaponTranslationScale,
        Yaw * Profile.WeaponTranslationScale * 0.12f,
        Pitch * Profile.WeaponTranslationScale * 0.08f);
    WeaponRecoilRotation.Pitch += Pitch * Profile.WeaponPitchScale;
    WeaponRecoilRotation.Yaw += Yaw * Profile.WeaponYawScale;
    WeaponRecoilRotation.Roll -= Yaw * Profile.WeaponRollScale;

    CameraRecoilRotation.Pitch += Pitch * Profile.CameraPitchScale;
    CameraRecoilRotation.Yaw += Yaw * Profile.CameraYawScale;
    CameraRecoilRotation.Roll -= Yaw * Profile.CameraRollScale;

    WeaponRecoilLocation.X = FMath::Clamp(WeaponRecoilLocation.X, -4.5f, 0.f);
    WeaponRecoilLocation.Y = FMath::Clamp(WeaponRecoilLocation.Y, -1.25f, 1.25f);
    WeaponRecoilLocation.Z = FMath::Clamp(WeaponRecoilLocation.Z, -0.5f, 1.5f);
    WeaponRecoilRotation.Pitch = FMath::Clamp(WeaponRecoilRotation.Pitch, 0.f, 7.f);
    WeaponRecoilRotation.Yaw = FMath::Clamp(WeaponRecoilRotation.Yaw, -2.5f, 2.5f);
    WeaponRecoilRotation.Roll = FMath::Clamp(WeaponRecoilRotation.Roll, -1.5f, 1.5f);
    CameraRecoilRotation.Pitch = FMath::Clamp(CameraRecoilRotation.Pitch, 0.f, 3.5f);
    CameraRecoilRotation.Yaw = FMath::Clamp(CameraRecoilRotation.Yaw, -1.25f, 1.25f);
    CameraRecoilRotation.Roll = FMath::Clamp(CameraRecoilRotation.Roll, -0.75f, 0.75f);

    if (ShotSound) UGameplayStatics::PlaySoundAtLocation(this, ShotSound, Result.TraceStart);
}

float UTUWeaponPresentationComponent::PhaseAnimationFraction(ETUWeaponActionPhase Phase, float Remaining, float Duration)
{
    const float Segment = FMath::Clamp(static_cast<int32>(Phase) - 1, 0, 4);
    return (Segment + FMath::Clamp(1.f - Remaining / FMath::Max(Duration, .001f), 0.f, 1.f)) / 5.f;
}
void UTUWeaponPresentationComponent::UpdateEvaluatedAnimation(float DeltaTime)
{
    auto* Operator = Cast<ATU_OperatorCharacter>(GetOwner());
    if (!Operator || !IsValid(ActiveWeapon)) return;
    const auto& State = ActiveWeapon->GetActionState();
    const float Now = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    if (State.bActive && PrototypeReload)
    {
        const float Target = PhaseAnimationFraction(State.Phase, State.PhaseEndServerTime - Now, ActiveWeapon->GetPresentationPhaseDuration()) * PrototypeReload->GetPlayLength();
        EvaluatedAnimationTime = FMath::Max(EvaluatedAnimationTime, Target);
    }
    ReloadWeight = FMath::Lerp(ReloadWeight, State.bActive ? 1.f : 0.f, 1.f - FMath::Exp(-16.f * FMath::Max(DeltaTime, 0.f)));
    const UStaticMeshComponent* WeaponMesh = ActiveWeapon->GetWeaponBodyMesh();
    bCalibratedContacts = WeaponMesh && WeaponMesh->GetStaticMesh() == PrototypeRifle;
    // Measured MF_Rifle_Idle_ADS time-zero component-space samples, handling-assets.json.
    // Epic ShooterCharacter attaches the rifle to HandGrip_R with SnapToTarget.
    // Thus hand bone relative to that actual socket is the required mesh-space IK target.
    const FTransform RestRight(FQuat(-.0386802426, .169085932, -.633582251, .753980980), FVector(-16.8238128, 7.09737698, 139.904362));
    const FTransform RestLeft(FQuat(.451083899, .822524815, -.310249447, -.154017938), FVector(-7.62345367, 43.2806608, 141.581820));
    const FTransform GripSocket(FQuat(.0922137539, .146911631, .0851361490, .981155152), FVector(-15.8746737, 14.2863839, 140.790158));
    FTransform WorldRight = FTransform::Identity, WorldLeft = FTransform::Identity;
    FTransform OwnerRight = FTransform::Identity, OwnerLeft = FTransform::Identity;
    auto* Parts = ActiveWeapon->PartsPresentation.Get();
    auto ComputeContacts = [&](const FTransform& VisibleMesh, const FTransform& ContactActor,
        FTransform& OutRight, FTransform& OutLeft)
    {
        OutRight = RestRight.GetRelativeTransform(GripSocket) * VisibleMesh;
        OutLeft = RestLeft.GetRelativeTransform(GripSocket) * VisibleMesh;
        OutLeft.AddToTranslation(ContactActor.TransformVectorNoScale(FVector(-2.f,3.f,6.f)));
        if (Parts && Parts->IsSupported())
        {
            const auto& Frame = Parts->GetFrame();
            const FTransform MagazineGrip = Frame.HandInActor * ContactActor;
            FTransform Blended; Blended.Blend(OutLeft, MagazineGrip, Frame.HandContactAlpha);
            OutLeft = Blended;
        }
    };
    if (bCalibratedContacts)
    {
        const bool bParts = Parts && Parts->IsSupported();
        const FTransform WorldMesh = bParts ? Parts->GetVisibleMeshWorld() : WeaponMesh->GetComponentTransform();
        const FTransform WorldActor = bParts ? Parts->GetVisibleActorWorld() : ActiveWeapon->GetActorTransform();
        const FTransform OwnerMesh = bParts ? Parts->GetOwnerVisibleMeshWorld() : WorldMesh;
        const FTransform OwnerActor = bParts ? Parts->GetOwnerVisibleActorWorld() : WorldActor;
        ComputeContacts(WorldMesh, WorldActor, WorldRight, WorldLeft);
        ComputeContacts(OwnerMesh, OwnerActor, OwnerRight, OwnerLeft);
        RightGripWorld = OwnerRight; LeftGripWorld = OwnerLeft;
    }
    for (USkeletalMeshComponent* Body : {Operator->GetMesh(), Operator->GetOwnerBodyMesh()})
    {
        if (!Body) continue;
        if (auto* Anim = Cast<UTUHandlingAnimInstance>(Body->GetAnimInstance()))
        {
            const bool bOwnerBody = Body == Operator->GetOwnerBodyMesh();
            const FTransform ActorFrame = Parts && Parts->IsSupported()
                ? (bOwnerBody ? Parts->GetOwnerVisibleActorWorld() : Parts->GetVisibleActorWorld())
                : ActiveWeapon->GetActorTransform();
            Anim->Idle = PrototypeIdle; Anim->Reload = PrototypeReload;
            Anim->IdleTime = PrototypeIdle ? FMath::Fmod(MotionTime, static_cast<double>(PrototypeIdle->GetPlayLength())) : 0.f;
            Anim->ActionTime = EvaluatedAnimationTime; Anim->ActionWeight = ReloadWeight;
            Anim->bContactsValid = bCalibratedContacts;
            Anim->MagazineGripAlpha=Parts && Parts->IsSupported()?Parts->GetFrame().MagazineGripAlpha:0.f;
            Anim->ControlGripAlpha=Parts && Parts->IsSupported()?Parts->GetFrame().ControlGripAlpha:0.f;
            Anim->MagazineObjectWorld=Parts && Parts->IsSupported()?Parts->GetFrame().MovingInActor*ActorFrame:ActiveWeapon->GetActorTransform();
            Anim->bOverrideSupportTrajectory = State.bActive && Parts && Parts->IsSupported();
            Anim->ReloadClipBodyWeightScale = TUWeaponContact::ResolveMountProfile(ActiveWeapon->GetWeaponDefinition().WeaponId).ReloadClipBodyWeight;
            const auto* Defaults = Operator->GetClass()->GetDefaultObject<ATU_OperatorCharacter>();
            Anim->CrouchDrop = 2.f * FMath::Max(0.f, Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - Operator->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
            const FQuat ComponentRotation = Body->GetComponentQuat();
            const FQuat AimDeltaWorld = Operator->GetHandlingEyeWorld().GetRotation() * Operator->GetActorQuat().Inverse();
            Anim->AimRotationComponent = ComponentRotation.Inverse() * AimDeltaWorld * ComponentRotation;
            Anim->EyeRotationComponent = ComponentRotation.Inverse() * Operator->GetHandlingEyeWorld().GetRotation();
            Anim->RightGripWorld = bOwnerBody ? OwnerRight : WorldRight;
            Anim->LeftGripWorld = bOwnerBody ? OwnerLeft : WorldLeft;
            Anim->RestRightHand = RestRight; Anim->RestLeftHand = RestLeft;
        }
    }
}
