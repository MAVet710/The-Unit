#include "TU_WeaponBase.h"
#include "TUWeaponComponent.h"

ATU_WeaponBase::ATU_WeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMechanics = CreateDefaultSubobject<UTUWeaponComponent>(TEXT("WeaponMechanics"));
    bCanFire = true;
    bIsReloading = false;
    AvailableFireModes = {ETUFireMode::SemiAuto, ETUFireMode::Burst, ETUFireMode::FullAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 3;
    ShotsRemainingInBurst = 0;
    bIsFiring = false;
}

void ATU_WeaponBase::Fire()
{
    StartFire();
}

bool ATU_WeaponBase::CanFire() const
{
    return bCanFire && !bIsReloading && WeaponMechanics->HasAmmo()
        && (!WeaponMechanics->WeaponDefinition.bSemiAutoOnly || CurrentFireMode == ETUFireMode::SemiAuto);
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

    WeaponMechanics->ConsumeRound();
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
    if (AvailableFireModes.Contains(NewFireMode)
        && (!WeaponMechanics->WeaponDefinition.bSemiAutoOnly || NewFireMode == ETUFireMode::SemiAuto))
    {
        CurrentFireMode = NewFireMode;
    }
}

void ATU_WeaponBase::CycleFireMode()
{
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

    CurrentFireMode = AvailableFireModes[NextIndex];
}

void ATU_WeaponBase::StartReload()
{
    if (bIsReloading)
    {
        return;
    }

    if (!WeaponMechanics->CanReload())
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

    WeaponMechanics->Reload();
    bIsReloading = false;
}

void ATU_WeaponBase::AddReserveAmmo(int32 Amount)
{
    WeaponMechanics->AddReserveAmmo(Amount);
}

int32 ATU_WeaponBase::GetCurrentAmmo() const
{
    const FMagazineState& Magazine = WeaponMechanics->MagazineState;
    return Magazine.RoundsInMagazine + (Magazine.bRoundChambered ? 1 : 0);
}

int32 ATU_WeaponBase::GetReserveAmmo() const
{
    return WeaponMechanics->AmmoReserve;
}

FMagazineState ATU_WeaponBase::GetMagazineState() const
{
    return WeaponMechanics->MagazineState;
}

FWeaponDefinition ATU_WeaponBase::GetWeaponDefinition() const
{
    return WeaponMechanics->WeaponDefinition;
}

FAmmoDefinition ATU_WeaponBase::GetAmmoDefinition() const
{
    return WeaponMechanics->AmmoDefinition;
}
