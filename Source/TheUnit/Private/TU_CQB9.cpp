#include "TU_CQB9.h"
ATU_CQB9::ATU_CQB9()
{
    bUseTimedFireCadence=true; ReloadDurationSeconds=2.2f; TraceRangeCm=85000.f;
    AvailableFireModes={ETUFireMode::SemiAuto,ETUFireMode::FullAuto}; CurrentFireMode=ETUFireMode::SemiAuto;
    FWeaponDefinition W; W.WeaponId=TEXT("WPN_CQB9"); W.DisplayName=FText::FromString(TEXT("CQB-9"));
    W.RecoilPitch=.72f; W.RecoilYaw=.31f; W.FireRateRPM=800.f; W.bSemiAutoOnly=false; W.HipSpread=1.55f; W.ADSSpread=.31f; W.CompatibleAmmoId=TEXT("Ammo_TU9_Subsonic");
    FAmmoDefinition A; A.AmmoId=TEXT("Ammo_TU9_Subsonic"); A.Damage=27.f; A.Penetration=7.f; A.Velocity=330.f; A.ArmorDamage=8.f; A.BleedChance=.07f;
    FMagazineState M; M.Capacity=30; M.RoundsInMagazine=29; M.bRoundChambered=true;
    ConfigureWeaponDefaults(W,A,M,120);
}
