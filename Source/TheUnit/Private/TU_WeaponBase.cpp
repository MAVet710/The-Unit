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
    if (!CanFire())
    {
        return;
    }

    ShotsRemainingInBurst = FMath::Min(BurstCount, CurrentAmmoInMagazine);

    while (ShotsRemainingInBurst > 0 && CanFire())
    {
        FireSingleShot();
        --ShotsRemainingInBurst;
    }
}

void ATU_WeaponBase::HandleFullAutoFire()
{
    if (!CanFire())
    {
        bIsFiring = false;
        return;
    }

    bIsFiring = true;

    // Placeholder: full auto timer-based repeated shots will be added in a later phase.
    FireSingleShot();
}

bool ATU_WeaponBase::CanFire() const
{
    return bCanFire && !bIsReloading && CurrentAmmoInMagazine > 0;
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

    bIsFiring = false;
    bIsReloading = true;
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

    const int32 CurrentIndex = AvailableFireModes.Find(CurrentFireMode);
    const int32 NextIndex = (CurrentIndex == INDEX_NONE) ? 0 : (CurrentIndex + 1) % AvailableFireModes.Num();
    CurrentFireMode = AvailableFireModes[NextIndex];
}
