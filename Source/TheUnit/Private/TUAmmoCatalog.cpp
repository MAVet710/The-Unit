#include "TUAmmoCatalog.h"
namespace { FAmmoDefinition Ammo(const TCHAR* Id,float D,float P,float V,float A,float B){FAmmoDefinition X;X.AmmoId=Id;X.Damage=D;X.Penetration=P;X.Velocity=V;X.ArmorDamage=A;X.BleedChance=B;return X;} }
TArray<FAmmoDefinition> UTUAmmoCatalog::BetaDefinitions()
{
    return {
        Ammo(TEXT("Ammo_TU556_Ball"),34,12,870,13,.07f),
        Ammo(TEXT("Ammo_TU545_Ball"),30,11,760,13,.09f),
        Ammo(TEXT("Ammo_TU762_Precision"),48,18,800,19,.12f),
        Ammo(TEXT("Ammo_TU9_Ball"),28,7,360,8,.06f),
        Ammo(TEXT("Ammo_TU57_Ball"),25,13,650,12,.05f),
        Ammo(TEXT("Ammo_TU9_Subsonic"),27,7,330,8,.07f),
        Ammo(TEXT("Ammo_TU556_Barrier"),31,17,825,18,.06f),
        Ammo(TEXT("Ammo_TU762_Training"),40,10,760,11,.05f)
    };
}
bool UTUAmmoCatalog::Find(FName Id,FAmmoDefinition& Out){for(const auto& A:BetaDefinitions())if(A.AmmoId==Id){Out=A;return true;}return false;}
