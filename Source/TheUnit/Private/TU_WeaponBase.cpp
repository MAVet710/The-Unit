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
}

void ATU_WeaponBase::Fire()
{
    if (!CanFire())
    {
        return;
    }

    CurrentAmmoInMagazine = FMath::Max(0, CurrentAmmoInMagazine - 1);
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
