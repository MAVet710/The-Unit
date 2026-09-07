#include "TUWeaponComponent.h"

UTUWeaponComponent::UTUWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    // Preserve the actor's initial 30 loaded rounds, including one in the chamber.
    MagazineState.RoundsInMagazine = 29;
    WeaponDefinition.DisplayName = FText::FromString(TEXT("Weapon"));
    WeaponDefinition.FireRateRPM = 600.0f;
    WeaponDefinition.bSemiAutoOnly = false;
    AmmoDefinition.Damage = 25.0f;
}

bool UTUWeaponComponent::HasAmmo() const
{
    return MagazineState.bRoundChambered || MagazineState.RoundsInMagazine > 0;
}

bool UTUWeaponComponent::ConsumeRound()
{
    if (!HasAmmo())
    {
        return false;
    }

    // Feed before firing if necessary; every successful shot spends one round.
    if (!MagazineState.bRoundChambered)
    {
        --MagazineState.RoundsInMagazine;
    }
    MagazineState.bRoundChambered = false;
    if (MagazineState.RoundsInMagazine > 0)
    {
        --MagazineState.RoundsInMagazine;
        MagazineState.bRoundChambered = true;
    }
    return true;
}

bool UTUWeaponComponent::CanReload() const
{
    return AmmoReserve > 0 && MagazineState.Capacity > 0
        && MagazineState.RoundsInMagazine < MagazineState.Capacity;
}

void UTUWeaponComponent::Reload()
{
    if (!CanReload())
    {
        return;
    }

    const int32 Needed = MagazineState.Capacity - MagazineState.RoundsInMagazine;
    const int32 ToLoad = FMath::Min(Needed, AmmoReserve);
    MagazineState.RoundsInMagazine += ToLoad;
    AmmoReserve -= ToLoad;
    if (!MagazineState.bRoundChambered && MagazineState.RoundsInMagazine > 0)
    {
        --MagazineState.RoundsInMagazine;
        MagazineState.bRoundChambered = true;
    }
}

void UTUWeaponComponent::AddReserveAmmo(int32 Amount)
{
    if (Amount > 0)
    {
        AmmoReserve += FMath::Min(Amount, MAX_int32 - AmmoReserve);
    }
}
