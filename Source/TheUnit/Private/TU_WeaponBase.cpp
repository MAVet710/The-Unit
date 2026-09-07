#include "TU_WeaponBase.h"

#include "TUWeaponAttachmentComponent.h"
#include "TUWeaponComponent.h"
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

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    SetRootComponent(WeaponRoot);

    WeaponBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBodyMesh"));
    WeaponBodyMesh->SetupAttachment(WeaponRoot);
    WeaponBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponBodyMesh->SetGenerateOverlapEvents(false);
    WeaponBodyMesh->SetOnlyOwnerSee(true);
    WeaponBodyMesh->SetCastShadow(false);

    AttachmentComponent = CreateDefaultSubobject<UTUWeaponAttachmentComponent>(TEXT("WeaponAttachments"));
    WeaponMechanics = CreateDefaultSubobject<UTUWeaponComponent>(TEXT("WeaponMechanics"));

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
    if (!bCanFire || bIsReloading || !WeaponMechanics || !WeaponMechanics->HasAmmo())
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
    bIsFiring = false;
    ShotsRemainingInBurst = 0;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimerHandle);
    }
}

void ATU_WeaponBase::FireSingleShot()
{
    if (!CanFire())
    {
        return;
    }

    if (!WeaponMechanics->ConsumeRound())
    {
        return;
    }

    if (bUseTimedFireCadence)
    {
        if (const UWorld* World = GetWorld())
        {
            NextAllowedFireTimeSeconds = World->GetTimeSeconds() + GetFireIntervalSeconds();
        }
    }

    PerformHitscanShot();
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

    FireSingleShot();
    --ShotsRemainingInBurst;

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

    FireSingleShot();
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

void ATU_WeaponBase::StartReload()
{
    if (bIsReloading || !WeaponMechanics || !WeaponMechanics->CanReload())
    {
        return;
    }

    bIsReloading = true;
    StopFire();

    if (bUseTimedFireCadence && ReloadDurationSeconds > 0.0f && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            ReloadTimerHandle,
            this,
            &ATU_WeaponBase::FinishReload,
            ReloadDurationSeconds,
            false);
    }
    else
    {
        FinishReload();
    }
}

void ATU_WeaponBase::FinishReload()
{
    if (!bIsReloading || !WeaponMechanics)
    {
        return;
    }

    WeaponMechanics->Reload();
    bIsReloading = false;
}

void ATU_WeaponBase::AddReserveAmmo(int32 Amount)
{
    if (WeaponMechanics)
    {
        WeaponMechanics->AddReserveAmmo(Amount);
    }
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
    const FMagazineState& Magazine = WeaponMechanics->MagazineState;
    return Magazine.RoundsInMagazine + (Magazine.bRoundChambered ? 1 : 0);
}

int32 ATU_WeaponBase::GetReserveAmmo() const
{
    return WeaponMechanics ? WeaponMechanics->AmmoReserve : 0;
}

FMagazineState ATU_WeaponBase::GetMagazineState() const
{
    return WeaponMechanics ? WeaponMechanics->MagazineState : FMagazineState();
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
        OnShotFired.Broadcast(LastShotResult);
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

    LastShotResult.TraceStart = ViewLocation;
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

    OnShotFired.Broadcast(LastShotResult);
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
            GetFireIntervalSeconds(),
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
            GetFireIntervalSeconds(),
            false);
    }
}
