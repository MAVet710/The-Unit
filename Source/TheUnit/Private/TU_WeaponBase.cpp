#include "TU_WeaponBase.h"

ATU_WeaponBase::ATU_WeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponDisplayName = FText::FromString(TEXT("Weapon"));
    MagazineCapacity = 30;
    CurrentAmmoInMagazine = MagazineCapacity;
    ReserveAmmo = 90;
    FireRate = 10.0f;
    Damage = 25.0f;
    bCanFire = true;
    bIsReloading = false;
    AvailableFireModes = {ETUFireMode::SemiAuto, ETUFireMode::Burst, ETUFireMode::FullAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 3;
    ShotsRemainingInBurst = 0;
    TimeBetweenShots = 0.1f;
    bIsFiring = false;
}

void ATU_WeaponBase::Fire()
{
    StartFire();
}

bool ATU_WeaponBase::CanFire() const
{
    return bCanFire && !bIsReloading && CurrentAmmoInMagazine > 0;
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
}

void ATU_WeaponBase::FireSingleShot()
{
    if (!CanFire())
    {
        return;
    }

    CurrentAmmoInMagazine = FMath::Max(0, CurrentAmmoInMagazine - 1);
}

void ATU_WeaponBase::HandleBurstFire()
{
    ShotsRemainingInBurst = FMath::Max(0, BurstCount);

    while (ShotsRemainingInBurst > 0 && CanFire())
    {
        FireSingleShot();
        --ShotsRemainingInBurst;
    }
}

void ATU_WeaponBase::HandleFullAutoFire()
{
    bIsFiring = true;
    FireSingleShot();
}

ETUFireMode ATU_WeaponBase::GetCurrentFireMode() const
{
    return CurrentFireMode;
}

void ATU_WeaponBase::SetFireMode(ETUFireMode NewFireMode)
{
    if (AvailableFireModes.Contains(NewFireMode))
    {
        CurrentFireMode = NewFireMode;
    }
}

void ATU_WeaponBase::CycleFireMode()
{
    if (AvailableFireModes.Num() == 0)
    {
        return;
    }

    const int32 CurrentIndex = AvailableFireModes.IndexOfByKey(CurrentFireMode);
    const int32 NextIndex = (CurrentIndex == INDEX_NONE)
        ? 0
        : (CurrentIndex + 1) % AvailableFireModes.Num();

    CurrentFireMode = AvailableFireModes[NextIndex];
}

void ATU_WeaponBase::StartReload()
{
    if (bIsReloading)
    {
        return;
    }

    if (CurrentAmmoInMagazine >= MagazineCapacity || ReserveAmmo <= 0)
    {
        return;
    }

    bIsReloading = true;
    StopFire();
    FinishReload();
}

void ATU_WeaponBase::FinishReload()
{
    if (!bIsReloading)
    {
        return;
    }

    const int32 AmmoNeeded = FMath::Max(0, MagazineCapacity - CurrentAmmoInMagazine);
    const int32 AmmoToLoad = FMath::Min(AmmoNeeded, ReserveAmmo);

    CurrentAmmoInMagazine += AmmoToLoad;
    ReserveAmmo -= AmmoToLoad;
    bIsReloading = false;
}

void ATU_WeaponBase::AddReserveAmmo(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    ReserveAmmo += Amount;
}

int32 ATU_WeaponBase::GetCurrentAmmo() const
{
    return CurrentAmmoInMagazine;
}

int32 ATU_WeaponBase::GetReserveAmmo() const
{
    return ReserveAmmo;
}
