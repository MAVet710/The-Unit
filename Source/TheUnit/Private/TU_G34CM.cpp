#include "TU_G34CM.h"

ATU_G34CM::ATU_G34CM()
{
    bUseTimedFireCadence = true;
    ReloadDurationSeconds = 1.75f;
    TraceRangeCm = 75000.0f;
    AvailableFireModes = {ETUFireMode::SemiAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 1;

    FWeaponDefinition WeaponDefinition;
    WeaponDefinition.WeaponId = TEXT("WPN_G34CM_CompetitionPistol");
    WeaponDefinition.DisplayName = FText::FromString(TEXT("G34 CM Competition Pistol"));
    WeaponDefinition.RecoilPitch = 0.78f;
    WeaponDefinition.RecoilYaw = 0.24f;
    WeaponDefinition.FireRateRPM = 430.0f;
    WeaponDefinition.bSemiAutoOnly = true;
    WeaponDefinition.HipSpread = 1.85f;
    WeaponDefinition.ADSSpread = 0.24f;
    WeaponDefinition.CompatibleAmmoId = TEXT("Ammo_TU9_Ball");

    FAmmoDefinition AmmoDefinition;
    AmmoDefinition.AmmoId = TEXT("Ammo_TU9_Ball");
    AmmoDefinition.Damage = 26.0f;
    AmmoDefinition.Penetration = 8.0f;
    AmmoDefinition.Velocity = 360.0f;
    AmmoDefinition.ArmorDamage = 8.0f;
    AmmoDefinition.BleedChance = 0.05f;

    FMagazineState MagazineState;
    MagazineState.Capacity = 20;
    MagazineState.RoundsInMagazine = 19;
    MagazineState.bRoundChambered = true;

    ConfigureWeaponDefaults(WeaponDefinition, AmmoDefinition, MagazineState, 100);
}
