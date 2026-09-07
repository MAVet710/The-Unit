#include "TU_AK105.h"

ATU_AK105::ATU_AK105()
{
    bUseTimedFireCadence = true;
    ReloadDurationSeconds = 2.65f;
    TraceRangeCm = 110000.0f;
    AvailableFireModes = {ETUFireMode::SemiAuto, ETUFireMode::FullAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 3;

    FWeaponDefinition WeaponDefinition;
    WeaponDefinition.WeaponId = TEXT("WPN_AK105_Modernized");
    WeaponDefinition.DisplayName = FText::FromString(TEXT("AK-105 Modernized"));
    WeaponDefinition.RecoilPitch = 1.18f;
    WeaponDefinition.RecoilYaw = 0.46f;
    WeaponDefinition.FireRateRPM = 650.0f;
    WeaponDefinition.bSemiAutoOnly = false;
    WeaponDefinition.HipSpread = 1.82f;
    WeaponDefinition.ADSSpread = 0.36f;
    WeaponDefinition.CompatibleAmmoId = TEXT("Ammo_TU545_Ball");

    FAmmoDefinition AmmoDefinition;
    AmmoDefinition.AmmoId = TEXT("Ammo_TU545_Ball");
    AmmoDefinition.Damage = 30.0f;
    AmmoDefinition.Penetration = 11.0f;
    AmmoDefinition.Velocity = 760.0f;
    AmmoDefinition.ArmorDamage = 13.0f;
    AmmoDefinition.BleedChance = 0.09f;

    FMagazineState MagazineState;
    MagazineState.Capacity = 30;
    MagazineState.RoundsInMagazine = 29;
    MagazineState.bRoundChambered = true;

    ConfigureWeaponDefaults(WeaponDefinition, AmmoDefinition, MagazineState, 120);
}
