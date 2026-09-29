#include "TU_WeaponBase.h"
#include "TUProjectileWorldSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Math/RotationMatrix.h"

#include "TUWeaponAttachmentComponent.h"
#include "TUWeaponComponent.h"
#include "TUWeaponPartsComponent.h"
#include "TUInventoryAmmoRegistry.h"
#include "Net/UnrealNetwork.h"
#include "TUWorldItem.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TUWeaponLoadoutData.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATU_WeaponBase::ATU_WeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    SetRootComponent(WeaponRoot);

    WeaponBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBodyMesh"));
    WeaponBodyMesh->SetIsReplicated(true);
    WeaponBodyMesh->SetupAttachment(WeaponRoot);
    WeaponBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponBodyMesh->SetGenerateOverlapEvents(false);
    WeaponBodyMesh->SetOnlyOwnerSee(false);
    WeaponBodyMesh->SetCastShadow(false);

    AttachmentComponent = CreateDefaultSubobject<UTUWeaponAttachmentComponent>(TEXT("WeaponAttachments"));
    WeaponMechanics = CreateDefaultSubobject<UTUWeaponComponent>(TEXT("WeaponMechanics"));

    PartsPresentation = CreateDefaultSubobject<UTUWeaponPartsComponent>(TEXT("PartsPresentation"));
    bCanFire = true;
    bIsReloading = false;
    AvailableFireModes = {ETUFireMode::SemiAuto, ETUFireMode::Burst, ETUFireMode::FullAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 3;
    ShotsRemainingInBurst = 0;
    bIsFiring = false;
}

void ATU_WeaponBase::BeginPlay()
{
    Super::BeginPlay();
    WeaponMechanics->Hydrate();

    if (AttachmentComponent)
    {
        AttachmentComponent->InitializeVisualRoot(WeaponBodyMesh);
        if (DefaultAttachmentLoadout)
        {
            AttachmentComponent->ApplyLoadout(DefaultAttachmentLoadout);
        }
    }
}

void ATU_WeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimerHandle);
        World->GetTimerManager().ClearTimer(ReloadTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void ATU_WeaponBase::Fire()
{
    StartFire();
}

bool ATU_WeaponBase::CanFire() const
{
    if (const auto* Operator = Cast<ATU_OperatorCharacter>(GetOwner()); Operator && !Operator->IsWeaponRaised()) return false;
    if (WeaponMechanics) const_cast<UTUWeaponComponent*>(WeaponMechanics.Get())->Hydrate();
    if (!CanOwnerManipulate() || !bCanFire || !WeaponMechanics || !WeaponMechanics->HasAmmo() || IsMuzzleObstructed())
    {
        return false;
    }

    if (WeaponMechanics->WeaponDefinition.bSemiAutoOnly && CurrentFireMode != ETUFireMode::SemiAuto)
    {
        return false;
    }

    if (bUseTimedFireCadence)
    {
        if (const UWorld* World = GetWorld())
        {
            if (World->GetTimeSeconds() + KINDA_SMALL_NUMBER < NextAllowedFireTimeSeconds)
            {
                return false;
            }
        }
    }

    return true;
}

void ATU_WeaponBase::StartFire()
{
    if (!HasAuthority()) { ServerStartFire(); return; }
    // Repeated held-trigger intent never restarts an active cadence or burst.
    if (bIsFiring || !CanFire()) return;
    if (GetWorld()) NextAllowedFireTimeSeconds = GetWorld()->GetTimeSeconds();
    switch (CurrentFireMode)
    {
        case ETUFireMode::SemiAuto:
            FireSingleShot();
            break;
        case ETUFireMode::Burst:
            HandleBurstFire();
            break;
        case ETUFireMode::FullAuto:
            HandleFullAutoFire();
            break;
        default:
            FireSingleShot();
            break;
    }
}

void ATU_WeaponBase::StopFire()
{
    if (!HasAuthority()) ServerStopFire();
    bIsFiring = false;
    ShotsRemainingInBurst = 0;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimerHandle);
    }
}

void ATU_WeaponBase::FireSingleShot()
{
    if (!HasAuthority()) { ServerStartFire(); return; }
    if (!CanFire())
    {
        return;
    }

    // The world admits the immutable shot before the existing ledger spends it.
    if (bUseProjectileFlight ? !LaunchPhysicalProjectile() : !WeaponMechanics->ConsumeRound())
    {
        return;
    }

    if (bUseTimedFireCadence)
    {
        if (const UWorld* World = GetWorld())
        {
            NextAllowedFireTimeSeconds = (bIsFiring ? NextAllowedFireTimeSeconds : World->GetTimeSeconds()) + GetFireIntervalSeconds();
        }
    }

    if (!bUseProjectileFlight) PerformHitscanShot();
    ApplyRecoil();
}

void ATU_WeaponBase::HandleBurstFire()
{
    if (!bUseTimedFireCadence)
    {
        ShotsRemainingInBurst = FMath::Max(0, BurstCount);
        while (ShotsRemainingInBurst > 0 && CanFire())
        {
            FireSingleShot();
            --ShotsRemainingInBurst;
        }
        return;
    }

    if (!bIsFiring)
    {
        bIsFiring = true;
        ShotsRemainingInBurst = FMath::Max(0, BurstCount);
    }

    if (ShotsRemainingInBurst <= 0 || GetCurrentAmmo() <= 0)
    {
        StopFire();
        return;
    }

    const int32 BeforeShot = GetCurrentAmmo();
    FireSingleShot();
    if (GetCurrentAmmo() < BeforeShot) --ShotsRemainingInBurst;
    else if (!GetWorld() || GetWorld()->GetTimeSeconds() + KINDA_SMALL_NUMBER >= NextAllowedFireTimeSeconds) { StopFire(); return; }

    if (ShotsRemainingInBurst > 0 && GetCurrentAmmo() > 0)
    {
        ScheduleNextBurstShot();
    }
    else
    {
        StopFire();
    }
}

void ATU_WeaponBase::HandleFullAutoFire()
{
    if (!bUseTimedFireCadence)
    {
        bIsFiring = true;
        FireSingleShot();
        return;
    }

    if (!bIsFiring)
    {
        bIsFiring = true;
    }

    if (GetCurrentAmmo() <= 0 || bIsReloading)
    {
        StopFire();
        return;
    }

    const int32 BeforeShot = GetCurrentAmmo();
    FireSingleShot();
    if (GetCurrentAmmo() == BeforeShot && (!GetWorld() || GetWorld()->GetTimeSeconds() + KINDA_SMALL_NUMBER >= NextAllowedFireTimeSeconds)) { StopFire(); return; }
    if (bIsFiring && GetCurrentAmmo() > 0)
    {
        ScheduleNextFullAutoShot();
    }
}

ETUFireMode ATU_WeaponBase::GetCurrentFireMode() const
{
    return CurrentFireMode;
}

void ATU_WeaponBase::SetFireMode(ETUFireMode NewFireMode)
{
    if (!HasAuthority()) { ServerSetFireMode(NewFireMode); return; }
    if (!CanOwnerManipulate()) return;
    if (!WeaponMechanics)
    {
        return;
    }

    if (AvailableFireModes.Contains(NewFireMode)
        && (!WeaponMechanics->WeaponDefinition.bSemiAutoOnly || NewFireMode == ETUFireMode::SemiAuto))
    {
        StopFire();
        CurrentFireMode = NewFireMode;
    }
}

void ATU_WeaponBase::CycleFireMode()
{
    if (!WeaponMechanics)
    {
        return;
    }

    if (WeaponMechanics->WeaponDefinition.bSemiAutoOnly)
    {
        SetFireMode(ETUFireMode::SemiAuto);
        return;
    }

    if (AvailableFireModes.Num() == 0)
    {
        return;
    }

    const int32 CurrentIndex = AvailableFireModes.IndexOfByKey(CurrentFireMode);
    const int32 NextIndex = (CurrentIndex == INDEX_NONE)
        ? 0
        : (CurrentIndex + 1) % AvailableFireModes.Num();

    SetFireMode(AvailableFireModes[NextIndex]);
}

void ATU_WeaponBase::StartReload() { RequestReload(FGuid::NewGuid(), ExportItemLedger().Revision, ETUReloadPolicy::Retain); }
void ATU_WeaponBase::FinishReload() { const auto A = GetActionState(); CommitActionPhase(A.ActionId, A.Revision); }
void ATU_WeaponBase::AddReserveAmmo(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0 || Amount > 10000 || WeaponMechanics->Action.bActive) return;
    WeaponMechanics->Hydrate();
    for (int32 I=0; I<Amount; ++I) WeaponMechanics->Ledger.LooseCartridges.Add(WeaponMechanics->AmmoDefinition.AmmoId);
    ++WeaponMechanics->Ledger.Revision; WeaponMechanics->Action.Revision = WeaponMechanics->Ledger.Revision;
}
float ATU_WeaponBase::GetFireIntervalSeconds() const
{
    const float RPM = WeaponMechanics ? WeaponMechanics->WeaponDefinition.FireRateRPM : 0.0f;
    return RPM > 0.0f ? 60.0f / RPM : 0.1f;
}

int32 ATU_WeaponBase::GetCurrentAmmo() const
{
    if (!WeaponMechanics)
    {
        return 0;
    }
    const FMagazineState Magazine = GetMagazineState();
    return Magazine.RoundsInMagazine + (Magazine.bRoundChambered ? 1 : 0);
}

int32 ATU_WeaponBase::GetReserveAmmo() const
{
    if (WeaponMechanics) const_cast<UTUWeaponComponent*>(WeaponMechanics.Get())->Hydrate();
    return WeaponMechanics ? WeaponMechanics->Reserve() : 0;
}

FMagazineState ATU_WeaponBase::GetMagazineState() const
{
    if (WeaponMechanics) const_cast<UTUWeaponComponent*>(WeaponMechanics.Get())->Hydrate();
    return WeaponMechanics ? WeaponMechanics->Snapshot() : FMagazineState();
}

FWeaponDefinition ATU_WeaponBase::GetWeaponDefinition() const
{
    return WeaponMechanics ? WeaponMechanics->WeaponDefinition : FWeaponDefinition();
}

FAmmoDefinition ATU_WeaponBase::GetAmmoDefinition() const
{
    return WeaponMechanics ? WeaponMechanics->AmmoDefinition : FAmmoDefinition();
}

void ATU_WeaponBase::ConfigureWeaponDefaults(
    const FWeaponDefinition& WeaponDefinition,
    const FAmmoDefinition& AmmoDefinition,
    const FMagazineState& MagazineState,
    int32 ReserveAmmo)
{
    if (!WeaponMechanics)
    {
        return;
    }

    WeaponMechanics->WeaponDefinition = WeaponDefinition;
    WeaponMechanics->AmmoDefinition = AmmoDefinition;
    WeaponMechanics->MagazineState = MagazineState;
    WeaponMechanics->AmmoReserve = FMath::Max(0, ReserveAmmo);
}

void ATU_WeaponBase::PerformHitscanShot()
{
    LastShotResult = FTUWeaponShotResult();
    LastShotResult.bFired = true;

    UWorld* World = GetWorld();
    if (!World)
    {
        MulticastShot(LastShotResult);
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    AController* Controller = OwnerPawn ? OwnerPawn->GetController() : GetInstigatorController();

    FVector ViewLocation = WeaponBodyMesh ? WeaponBodyMesh->GetComponentLocation() : GetActorLocation();
    FRotator ViewRotation = GetActorRotation();
    if (Controller)
    {
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }

    const float BaseSpreadDegrees = bIsAiming
        ? WeaponMechanics->WeaponDefinition.ADSSpread
        : WeaponMechanics->WeaponDefinition.HipSpread;
    const float AttachmentSpread = AttachmentComponent ? AttachmentComponent->GetSpreadMultiplier() : 1.0f;
    const float ConeRadians = FMath::DegreesToRadians(FMath::Max(0.0f, BaseSpreadDegrees * AttachmentSpread));
    const FVector ShotDirection = ConeRadians > KINDA_SMALL_NUMBER
        ? FMath::VRandCone(ViewRotation.Vector(), ConeRadians)
        : ViewRotation.Vector();

    LastShotResult.TraceStart = GetWorldMuzzleLocation();
    LastShotResult.TraceEnd = ViewLocation + ShotDirection * TraceRangeCm;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TUWeaponTrace), true, this);
    QueryParams.AddIgnoredActor(this);
    if (GetOwner())
    {
        QueryParams.AddIgnoredActor(GetOwner());
    }

    FHitResult Hit;
    if (World->LineTraceSingleByChannel(
        Hit,
        LastShotResult.TraceStart,
        LastShotResult.TraceEnd,
        ECC_Visibility,
        QueryParams))
    {
        LastShotResult.bHit = true;
        LastShotResult.ImpactPoint = Hit.ImpactPoint;
        LastShotResult.HitActor = Hit.GetActor();

        if (AActor* HitActor = Hit.GetActor())
        {
            UGameplayStatics::ApplyPointDamage(
                HitActor,
                WeaponMechanics->AmmoDefinition.Damage,
                ShotDirection,
                Hit,
                Controller,
                this,
                UDamageType::StaticClass());
        }
    }

    MulticastShot(LastShotResult);
}

void ATU_WeaponBase::ApplyRecoil()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !WeaponMechanics)
    {
        return;
    }

    const float AttachmentRecoil = AttachmentComponent ? AttachmentComponent->GetRecoilMultiplier() : 1.0f;
    const float Pitch = WeaponMechanics->WeaponDefinition.RecoilPitch * AttachmentRecoil;
    const float Yaw = WeaponMechanics->WeaponDefinition.RecoilYaw * AttachmentRecoil;

    OwnerPawn->AddControllerPitchInput(-Pitch);
    OwnerPawn->AddControllerYawInput(FMath::RandRange(-Yaw, Yaw));
}

void ATU_WeaponBase::ScheduleNextBurstShot()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FireTimerHandle,
            this,
            &ATU_WeaponBase::HandleBurstFire,
            FMath::Max(0.0001f, NextAllowedFireTimeSeconds - World->GetTimeSeconds()),
            false);
    }
}

void ATU_WeaponBase::ScheduleNextFullAutoShot()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FireTimerHandle,
            this,
            &ATU_WeaponBase::HandleFullAutoFire,
            FMath::Max(0.0001f, NextAllowedFireTimeSeconds - World->GetTimeSeconds()),
            false);
    }
}

FTUItemLedger ATU_WeaponBase::ExportItemLedger() const
{
    const_cast<UTUWeaponComponent*>(WeaponMechanics.Get())->Hydrate();
    auto L = WeaponMechanics->Ledger; L.WeaponActions = { WeaponMechanics->Action }; return L;
}
bool ATU_WeaponBase::ValidateItemLedger(const FTUItemLedger& Value) const { return WeaponMechanics && WeaponMechanics->ValidateImport(Value); }
bool ATU_WeaponBase::ImportItemLedger(const FTUItemLedger& Value)
{
    if (!HasAuthority() || !WeaponMechanics->Import(Value)) return false;
    StopFire(); if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
    bIsReloading = false; if (PartsPresentation) PartsPresentation->UpdatePresentation(0.f); ForceNetUpdate(); return true;
}
FGuid ATU_WeaponBase::GetWeaponInstanceId() const { const auto L = ExportItemLedger(); return L.Weapons.Num() ? L.Weapons[0].InstanceId : FGuid(); }
FTUWeaponActionState ATU_WeaponBase::GetActionState() const { return WeaponMechanics->Action; }
void ATU_WeaponBase::SetItemOwnerId(FGuid OwnerId)
{
    if (!HasAuthority() || !OwnerId.IsValid()) return;
    WeaponMechanics->Hydrate();
    for (auto& W : WeaponMechanics->Ledger.Weapons) W.OwnerId = OwnerId;
    for (auto& M : WeaponMechanics->Ledger.Magazines) M.OwnerId = OwnerId;
    for (auto& I : WeaponMechanics->Ledger.Items) I.OwnerId = OwnerId;
}
bool ATU_WeaponBase::RequestReload(FGuid Id, int32 Revision, ETUReloadPolicy Policy)
{
    if (!HasAuthority()) { ServerReload(Id, Revision, Policy); return false; }
    WeaponMechanics->Hydrate();
    if (!CanOwnerManipulate()) { ClientReconcile(ExportItemLedger(), GetActionState()); return false; }
    if (!WeaponMechanics->BeginReload(Id, Revision, Policy)) { ClientReconcile(ExportItemLedger(), GetActionState()); return false; }
    StopFire(); bIsReloading = true; WeaponMechanics->Action.PhaseEndServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f; ScheduleActionPhase(); if (PartsPresentation) PartsPresentation->UpdatePresentation(0.f); ForceNetUpdate(); return true;
}
bool ATU_WeaponBase::CommitActionPhase(FGuid Id, int32 Revision)
{
    if (!HasAuthority() || (GetWorld() && GetWorld()->GetTimeSeconds() + KINDA_SMALL_NUMBER < WeaponMechanics->Action.PhaseEndServerTime) || !WeaponMechanics->Commit(Id, Revision)) return false;
    for (int32 I = WeaponMechanics->Ledger.Magazines.Num()-1; I>=0; --I) {
        const auto M = WeaponMechanics->Ledger.Magazines[I];
        if (M.Location == ETUItemLocation::Ground && GetWorld()) {
            if (auto* Ground = GetWorld()->SpawnActor<ATUWorldItem>(PartsPresentation && PartsPresentation->IsSupported() ? PartsPresentation->GetVisibleMeshWorld().TransformPosition(FVector(0.f,14.3445f,-4.7425f)) : GetActorLocation(), FRotator::ZeroRotator)) {
                Ground->InitializeMagazine(M); WeaponMechanics->Ledger.Magazines.RemoveAt(I);
            } else { WeaponMechanics->Ledger.Magazines[I].Location = ETUItemLocation::Carried; }
        }
    }
    bIsReloading = WeaponMechanics->Action.bActive; ScheduleActionPhase();
    // Authority timers commit after the regular pose tick. Render the committed
    // item identity immediately so a dropped magazine never survives one frame.
    if (PartsPresentation) PartsPresentation->UpdatePresentation(0.f);
    ForceNetUpdate(); return true;
}
void ATU_WeaponBase::ScheduleActionPhase()
{
    if (!GetWorld()) return;
    GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
    if (!WeaponMechanics->Action.bActive) return;
    const float Duration = FMath::Max(.05f, ReloadDurationSeconds > 0.f ? ReloadDurationSeconds / 5.f : .2f);
    WeaponMechanics->Action.PhaseEndServerTime += Duration;
    const auto A = WeaponMechanics->Action;
    FTimerDelegate Callback; Callback.BindWeakLambda(this, [this,A](){ CommitActionPhase(A.ActionId,A.Revision); });
    GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, Callback, FMath::Max(0.0001f, A.PhaseEndServerTime - GetWorld()->GetTimeSeconds()), false);
}
void ATU_WeaponBase::InterruptWeaponAction()
{
    if (!HasAuthority()) { ServerInterrupt(GetActionState().ActionId, GetActionState().Revision); return; }
    StopFire(); if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
    WeaponMechanics->Interrupt(); bIsReloading = false; if (PartsPresentation) PartsPresentation->UpdatePresentation(0.f); ForceNetUpdate();
}
bool ATU_WeaponBase::CycleAction()
{
    if (!HasAuthority()) { ServerCycle(ExportItemLedger().Revision); return false; }
    if (!CanOwnerManipulate()) return false;
    WeaponMechanics->Hydrate(); const bool Result = WeaponMechanics->Cycle(); ForceNetUpdate(); return Result;
}
FVector ATU_WeaponBase::GetWorldMuzzleLocation() const
{
    return GetWorldMuzzleTransform().GetLocation();
}
bool ATU_WeaponBase::IsMuzzleObstructed() const
{
    if (!GetWorld()) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TUMuzzleObstruction), false, this); if (GetOwner()) Params.AddIgnoredActor(GetOwner());
    FHitResult Hit;
    return GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), GetWorldMuzzleLocation()+GetActorForwardVector()*5.f, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(3.f), Params);
}
bool ATU_WeaponBase::AcceptGroundMagazine(const FTUMagazineInstance& Magazine)
{
    if (!HasAuthority() || !TUInventoryAmmo::ValidMagazine(Magazine)) return false;
    WeaponMechanics->Hydrate(); if (WeaponMechanics->Action.bActive || WeaponMechanics->FindMagazine(Magazine.InstanceId)) return false;
    auto M = Magazine; M.OwnerId = WeaponMechanics->Ledger.Weapons[0].OwnerId; M.WeaponId = GetWeaponInstanceId(); M.Location = ETUItemLocation::Carried;
    WeaponMechanics->Ledger.Magazines.Add(M); ++WeaponMechanics->Ledger.Revision; WeaponMechanics->Action.Revision = WeaponMechanics->Ledger.Revision; ForceNetUpdate(); return true;
}
void ATU_WeaponBase::ServerStartFire_Implementation() { StartFire(); }
void ATU_WeaponBase::ServerStopFire_Implementation() { StopFire(); }
void ATU_WeaponBase::ServerReload_Implementation(FGuid Id,int32 Revision,ETUReloadPolicy Policy) { RequestReload(Id,Revision,Policy); }
void ATU_WeaponBase::ServerCycle_Implementation(int32 Revision) { if (Revision == ExportItemLedger().Revision) CycleAction(); ClientReconcile(ExportItemLedger(),GetActionState()); }
void ATU_WeaponBase::ServerInterrupt_Implementation(FGuid Id, int32 Revision) { if (Id == GetActionState().ActionId && Revision == GetActionState().Revision) InterruptWeaponAction(); else ClientReconcile(ExportItemLedger(),GetActionState()); }
void ATU_WeaponBase::ClientReconcile_Implementation(const FTUItemLedger& Ledger,const FTUWeaponActionState& Action) { WeaponMechanics->Ledger = Ledger; WeaponMechanics->Action = Action; bIsReloading = Action.bActive; if (PartsPresentation) PartsPresentation->UpdatePresentation(0.f); }
void ATU_WeaponBase::MulticastShot_Implementation(const FTUWeaponShotResult& Result) { LastShotResult = Result; OnShotFired.Broadcast(Result); }


bool ATU_WeaponBase::CanOwnerManipulate() const
{
    if (IsHidden()) return false;
    if (const auto* Operator = Cast<ATU_ArmedOperatorCharacter>(GetOwner())) return !Operator->IsCombatDisabled() && Operator->GetCurrentWeapon() == this;
    return true;
}

void ATU_WeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(ATU_WeaponBase, CurrentFireMode);
 DOREPLIFETIME(ATU_WeaponBase, bIsAiming);
}
void ATU_WeaponBase::SetAiming(bool bNewAiming)
{
 if (!HasAuthority()) { bIsAiming = bNewAiming; ServerSetAiming(bNewAiming); return; }
 bIsAiming = bNewAiming && CanOwnerManipulate();
}
void ATU_WeaponBase::ServerSetAiming_Implementation(bool bNewAiming) { SetAiming(bNewAiming); }
void ATU_WeaponBase::ServerSetFireMode_Implementation(ETUFireMode NewMode) { SetFireMode(NewMode); }
#if WITH_DEV_AUTOMATION_TESTS
bool ATU_WeaponBase::AdvanceActionClockForTesting()
{
    WeaponMechanics->Action.PhaseEndServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    return CommitActionPhase(GetActionState().ActionId,GetActionState().Revision);
}
#endif

FTransform ATU_WeaponBase::GetWorldMuzzleTransform() const
{
    if(WeaponBodyMesh&&WeaponBodyMesh->DoesSocketExist(MuzzleSocketName))return WeaponBodyMesh->GetSocketTransform(MuzzleSocketName);
    if(WeaponBodyMesh&&WeaponBodyMesh->GetStaticMesh()&&WeaponBodyMesh->GetStaticMesh()->GetPathName()==TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle"))
    {
        // Measured front-ring envelope from the installed mesh's OBJ export.
        // This is an explicit prototype geometry calibration, not an authored socket.
        const auto T=WeaponBodyMesh->GetComponentTransform();
        const FVector Forward=T.TransformVectorNoScale(FVector(0,1,0)).GetSafeNormal();
        return FTransform(FRotationMatrix::MakeFromXZ(Forward,T.TransformVectorNoScale(FVector::UpVector)).ToQuat(),T.TransformPosition(FVector(-.028,50.83,9.936)));
    }
    return FTransform(GetActorQuat(),GetActorLocation()+GetActorForwardVector()*65.f);
}
bool ATU_WeaponBase::LaunchPhysicalProjectile()
{
    UWorld* World=GetWorld();auto* Flight=World?World->GetSubsystem<UTUProjectileWorldSubsystem>():nullptr;
    if(!HasAuthority()||!Flight||!WeaponMechanics)return false;
    FTUProjectileLaunch Shot;Shot.ShotId=FGuid::NewGuid();Shot.WeaponId=GetWeaponInstanceId();
    const auto Ledger=ExportItemLedger();if(Ledger.Weapons.Num()!=1||Ledger.Weapons[0].ChamberAmmoId.IsNone())return false;
    Shot.AmmoId=Ledger.Weapons[0].ChamberAmmoId;
    Shot.ServerTime=World->GetTimeSeconds();Shot.Causer=this;Shot.Shooter=GetOwner();
    APawn* Pawn=Cast<APawn>(GetOwner());Shot.Controller=Pawn?Pawn->GetController():GetInstigatorController();
    const auto Muzzle=GetWorldMuzzleTransform();
    // Physics-disabled automation worlds still exercise authoritative ammo/cadence.
    // Skip only world-flight simulation while preserving the same atomic ammo spend.
    if (!World->GetPhysicsScene())
    {
        if (!WeaponMechanics->ConsumeRound()) return false;
        FTUWeaponShotResult Result; Result.bFired=true; Result.bSimulatedFlight=true; Result.ShotId=Shot.ShotId;
        Result.TraceStart=Muzzle.GetLocation(); Result.TraceEnd=Result.TraceStart+Muzzle.GetUnitAxis(EAxis::X)*TraceRangeCm;
        Result.LaunchServerTime=Shot.ServerTime; MulticastShot(Result); return true;
    }
    const double Spread=bIsAiming?WeaponMechanics->WeaponDefinition.ADSSpread:WeaponMechanics->WeaponDefinition.HipSpread;
    const double Multiplier=AttachmentComponent?AttachmentComponent->GetSpreadMultiplier():1.;
    FRandomStream Random(GetTypeHash(Shot.ShotId));
    const FVector Direction=Random.VRandCone(Muzzle.GetUnitAxis(EAxis::X),FMath::DegreesToRadians(FMath::Max(0.,Spread*Multiplier)));
    const double Speed=WeaponMechanics->AmmoDefinition.Velocity;
    if(!FMath::IsFinite(Speed)||Speed<=0.||!FMath::IsFinite(Spread)||!FMath::IsFinite(Multiplier))return false;
    Shot.Initial.PositionM=Muzzle.GetLocation()*.01;
    Shot.Initial.VelocityMps=Direction*Speed+(GetOwner()?GetOwner()->GetVelocity()*.01:FVector::ZeroVector);
    Shot.Environment.GravityMps2=FVector(0,0,World->GetGravityZ()*.01);
    Shot.MaxDistanceM=TraceRangeCm*.01;Shot.GameplayDamage=WeaponMechanics->AmmoDefinition.Damage;
    if(!Flight->TryLaunch(Shot,[this]{return WeaponMechanics->ConsumeRound();}))return false;
    FTUWeaponShotResult Result;Result.bFired=true;Result.bSimulatedFlight=true;Result.ShotId=Shot.ShotId;
    Result.TraceStart=Muzzle.GetLocation();Result.TraceEnd=Result.TraceStart+Direction*TraceRangeCm;
    Result.InitialVelocityMps=Shot.Initial.VelocityMps;Result.LaunchServerTime=Shot.ServerTime;
    MulticastShot(Result);return true;
}
void ATU_WeaponBase::ReportProjectileImpact(const FTUProjectileImpact& E)
{
    if(!HasAuthority())return;
    FTUWeaponShotResult Result;Result.ShotId=E.ShotId;Result.bHit=true;Result.bSimulatedFlight=true;
    Result.bFired=false;Result.ImpactPoint=E.Hit.ImpactPoint;Result.TraceEnd=E.Hit.ImpactPoint;
    Result.HitActor=E.Hit.GetActor();Result.FlightSeconds=E.FlightSeconds;Result.ImpactEnergyJoules=E.EnergyJoules;
    MulticastProjectileImpact(Result);
}
void ATU_WeaponBase::MulticastProjectileImpact_Implementation(const FTUWeaponShotResult& Result)
{
    // Deliberately separate: impacts must never replay a launch, recoil or report.
    OnProjectileImpact.Broadcast(Result);
}
