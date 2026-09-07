#include "TU_WeaponBase.h"
#include "TUWeaponBuildResolver.h"
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
    if (!bCanFire || bIsReloading || !WeaponMechanics->HasAmmo())
    {
        return false;
    }

    if (bHasActiveFireControl)
    {
        return AvailableFireModes.Contains(CurrentFireMode);
    }

    return !WeaponMechanics->WeaponDefinition.bSemiAutoOnly || CurrentFireMode == ETUFireMode::SemiAuto;
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
    if (!AvailableFireModes.Contains(NewFireMode))
    {
        return;
    }

    if (bHasActiveFireControl
        || !WeaponMechanics->WeaponDefinition.bSemiAutoOnly
        || NewFireMode == ETUFireMode::SemiAuto)
    {
        CurrentFireMode = NewFireMode;
    }
}

void ATU_WeaponBase::CycleFireMode()
{
    if (!bHasActiveFireControl && WeaponMechanics->WeaponDefinition.bSemiAutoOnly)
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

bool ATU_WeaponBase::ApplyResolvedBuild(const FTUResolvedWeaponBuild& ResolvedBuild, FString& OutFailureReason)
{
    OutFailureReason.Reset();

    if (!ResolvedBuild.bHasFireControl)
    {
        OutFailureReason = TEXT("Resolved modular weapon build has no fire-control module.");
        return false;
    }

    const FFireControlModuleDefinition& FireControl = ResolvedBuild.FireControlDefinition;
    if (FireControl.FireControlId.IsNone())
    {
        OutFailureReason = TEXT("Resolved fire-control definition has no stable identity.");
        return false;
    }

    TArray<ETUFireMode> SanitizedModes;
    for (ETUFireMode Mode : FireControl.SupportedFireModes)
    {
        SanitizedModes.AddUnique(Mode);
    }

    if (SanitizedModes.Num() == 0)
    {
        OutFailureReason = TEXT("Resolved fire-control definition exposes no supported fire mode.");
        return false;
    }

    if (SanitizedModes.Contains(ETUFireMode::Burst) && FireControl.BurstCount < 1)
    {
        OutFailureReason = TEXT("Burst-capable fire control requires a positive burst count.");
        return false;
    }

    if (FireControl.TriggerResponseMultiplier < 0.0f
        || FireControl.ResetResponseMultiplier < 0.0f
        || FireControl.SemiAutoResetDelaySeconds < 0.0f)
    {
        OutFailureReason = TEXT("Fire-control response values cannot be negative.");
        return false;
    }

    FWeaponDefinition NewWeaponDefinition = ResolvedBuild.DerivedWeaponDefinition;
    NewWeaponDefinition.bSemiAutoOnly = SanitizedModes.Num() == 1
        && SanitizedModes[0] == ETUFireMode::SemiAuto;

    StopFire();
    WeaponMechanics->WeaponDefinition = NewWeaponDefinition;
    bHasActiveFireControl = true;
    ActiveFireControlDefinition = FireControl;
    ActiveFireControlDefinition.SupportedFireModes = SanitizedModes;
    AvailableFireModes = SanitizedModes;
    BurstCount = FMath::Max(1, FireControl.BurstCount);

    if (!AvailableFireModes.Contains(CurrentFireMode))
    {
        CurrentFireMode = AvailableFireModes.Contains(ETUFireMode::SemiAuto)
            ? ETUFireMode::SemiAuto
            : AvailableFireModes[0];
    }

    return true;
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

TArray<ETUFireMode> ATU_WeaponBase::GetAvailableFireModes() const
{
    return AvailableFireModes;
}

bool ATU_WeaponBase::HasActiveFireControl() const
{
    return bHasActiveFireControl;
}

FName ATU_WeaponBase::GetActiveTriggerProfileId() const
{
    return bHasActiveFireControl ? ActiveFireControlDefinition.TriggerProfileId : NAME_None;
}

int32 ATU_WeaponBase::GetConfiguredBurstCount() const
{
    return BurstCount;
}

float ATU_WeaponBase::GetTriggerResponseMultiplier() const
{
    return bHasActiveFireControl ? ActiveFireControlDefinition.TriggerResponseMultiplier : 1.0f;
}

float ATU_WeaponBase::GetResetResponseMultiplier() const
{
    return bHasActiveFireControl ? ActiveFireControlDefinition.ResetResponseMultiplier : 1.0f;
}

float ATU_WeaponBase::GetSemiAutoResetDelaySeconds() const
{
    return bHasActiveFireControl ? ActiveFireControlDefinition.SemiAutoResetDelaySeconds : 0.0f;
}

bool ATU_WeaponBase::RequiresReleaseBetweenSemiShots() const
{
    return bHasActiveFireControl
        ? ActiveFireControlDefinition.bRequiresReleaseBetweenSemiShots
        : true;
}
