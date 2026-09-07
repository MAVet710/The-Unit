#include "TU_TacticalRifle.h"

ATU_TacticalRifle::ATU_TacticalRifle()
{
    bUseTimedFireCadence = true;
    ReloadDurationSeconds = 2.35f;
    TraceRangeCm = 120000.0f;
    AvailableFireModes = {ETUFireMode::SemiAuto, ETUFireMode::FullAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 3;

    FWeaponDefinition WeaponDefinition;
    WeaponDefinition.WeaponId = TEXT("WPN_TU556_ModularCarbine");
    WeaponDefinition.DisplayName = FText::FromString(TEXT("TU-556 Modular Carbine"));
    WeaponDefinition.RecoilPitch = 1.05f;
    WeaponDefinition.RecoilYaw = 0.38f;
    WeaponDefinition.FireRateRPM = 700.0f;
    WeaponDefinition.bSemiAutoOnly = false;
    WeaponDefinition.HipSpread = 1.65f;
    WeaponDefinition.ADSSpread = 0.32f;
    WeaponDefinition.CompatibleAmmoId = TEXT("Ammo_TU556_Ball");

    FAmmoDefinition AmmoDefinition;
    AmmoDefinition.AmmoId = TEXT("Ammo_TU556_Ball");
    AmmoDefinition.Damage = 32.0f;
    AmmoDefinition.Penetration = 12.0f;
    AmmoDefinition.Velocity = 800.0f;
    AmmoDefinition.ArmorDamage = 14.0f;
    AmmoDefinition.BleedChance = 0.08f;

    FMagazineState MagazineState;
    MagazineState.Capacity = 30;
    MagazineState.RoundsInMagazine = 29;
    MagazineState.bRoundChambered = true;

    ConfigureWeaponDefaults(WeaponDefinition, AmmoDefinition, MagazineState, 120);
}
