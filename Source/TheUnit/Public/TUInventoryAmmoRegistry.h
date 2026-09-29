#pragma once
#include "CoreMinimal.h"
#include "TUExecutionTypes.h"

/** Explicit immutable compatibility catalog. Variants currently share authored family ballistics. */
namespace TUInventoryAmmo
{
inline FName Family(FName Ammo)
{
 if (Ammo == TEXT("Ammo_556_Training_Ball") || Ammo == TEXT("Ammo_556_Training_Tracer") || Ammo == TEXT("Ammo_556_Training_Subsonic") || Ammo == TEXT("556")) return TEXT("Training556");
 if (Ammo == TEXT("Ammo_TU556_Ball")) return TEXT("TU556");
 if (Ammo == TEXT("Ammo_TU545_Ball")) return TEXT("TU545");
 if (Ammo == TEXT("Ammo_TU762_Precision")) return TEXT("TU762");
 if (Ammo == TEXT("Ammo_TU9_Ball")) return TEXT("TU9");
 if (Ammo == TEXT("Ammo_TU57_Ball")) return TEXT("TU57");
 return NAME_None;
}
inline bool Compatible(FName Ammo, FName FamilyKey) { return !Family(Ammo).IsNone() && Family(Ammo) == Family(FamilyKey); }
inline bool ValidMagazine(const FTUMagazineInstance& M)
{
 if (!M.InstanceId.IsValid() || M.DefinitionId.IsNone() || Family(M.CompatibleAmmoId).IsNone() || M.Capacity <= 0 || M.Capacity > 500 || M.Cartridges.Num() > M.Capacity) return false;
 for (FName Ammo : M.Cartridges) if (!Compatible(Ammo,M.CompatibleAmmoId)) return false;
 return true;
}
}
