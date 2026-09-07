#include "TU_M110.h"

ATU_M110::ATU_M110()
{
    bUseTimedFireCadence = true;
    ReloadDurationSeconds = 2.85f;
    TraceRangeCm = 180000.0f;
    AvailableFireModes = {ETUFireMode::SemiAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 1;

    FWeaponDefinition WeaponDefinition;
    WeaponDefinition.WeaponId = TEXT("WPN_M110_PrecisionDMR");
    WeaponDefinition.DisplayName = FText::FromString(TEXT("M110 Precision DMR"));
    WeaponDefinition.RecoilPitch = 1.42f;
    WeaponDefinition.RecoilYaw = 0.36f;
    WeaponDefinition.FireRateRPM = 310.0f;
    WeaponDefinition.bSemiAutoOnly = true;
    WeaponDefinition.HipSpread = 2.35f;
    WeaponDefinition.ADSSpread = 0.12f;
    WeaponDefinition.CompatibleAmmoId = TEXT("Ammo_TU762_Precision");

    FAmmoDefinition AmmoDefinition;
    AmmoDefinition.AmmoId = TEXT("Ammo_TU762_Precision");
    AmmoDefinition.Damage = 55.0f;
    AmmoDefinition.Penetration = 18.0f;
    AmmoDefinition.Velocity = 735.0f;
    AmmoDefinition.ArmorDamage = 20.0f;
    AmmoDefinition.BleedChance = 0.10f;

    FMagazineState MagazineState;
    MagazineState.Capacity = 20;
    MagazineState.RoundsInMagazine = 19;
    MagazineState.bRoundChambered = true;

    ConfigureWeaponDefaults(WeaponDefinition, AmmoDefinition, MagazineState, 80);
}
