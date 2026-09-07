#include "TU_RGRFive7.h"

ATU_RGRFive7::ATU_RGRFive7()
{
    bUseTimedFireCadence = true;
    ReloadDurationSeconds = 1.95f;
    TraceRangeCm = 80000.0f;
    AvailableFireModes = {ETUFireMode::SemiAuto};
    CurrentFireMode = ETUFireMode::SemiAuto;
    BurstCount = 1;

    FWeaponDefinition WeaponDefinition;
    WeaponDefinition.WeaponId = TEXT("WPN_RGRFive7_TacticalPistol");
    WeaponDefinition.DisplayName = FText::FromString(TEXT("RGR Five7 Tactical Pistol"));
    WeaponDefinition.RecoilPitch = 0.66f;
    WeaponDefinition.RecoilYaw = 0.21f;
    WeaponDefinition.FireRateRPM = 420.0f;
    WeaponDefinition.bSemiAutoOnly = true;
    WeaponDefinition.HipSpread = 1.75f;
    WeaponDefinition.ADSSpread = 0.28f;
    WeaponDefinition.CompatibleAmmoId = TEXT("Ammo_TU57_Ball");

    FAmmoDefinition AmmoDefinition;
    AmmoDefinition.AmmoId = TEXT("Ammo_TU57_Ball");
    AmmoDefinition.Damage = 24.0f;
    AmmoDefinition.Penetration = 10.0f;
    AmmoDefinition.Velocity = 520.0f;
    AmmoDefinition.ArmorDamage = 10.0f;
    AmmoDefinition.BleedChance = 0.045f;

    FMagazineState MagazineState;
    MagazineState.Capacity = 20;
    MagazineState.RoundsInMagazine = 19;
    MagazineState.bRoundChambered = true;

    ConfigureWeaponDefaults(WeaponDefinition, AmmoDefinition, MagazineState, 100);
}
