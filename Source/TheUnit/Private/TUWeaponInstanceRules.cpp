#include "TUWeaponInstanceRules.h"
#include "TUWeaponBuildResolver.h"
#include "TUWeaponDefinitionCatalog.h"

bool FTUWeaponInstanceRules::ValidateInstance(
    const FWeaponInstanceState& Instance,
    const UTUWeaponDefinitionCatalog* Catalog,
    FString& OutFailureReason)
{
    OutFailureReason.Reset();

    if (!Catalog)
    {
        OutFailureReason = TEXT("Weapon instance cannot be validated without a definition catalog.");
        return false;
    }

    if (!Instance.InstanceId.IsValid())
    {
        OutFailureReason = TEXT("Weapon instance has no valid persistent identity.");
        return false;
    }

    if (Instance.ConditionNormalized < 0.0f || Instance.ConditionNormalized > 1.0f)
    {
        OutFailureReason = TEXT("Weapon instance condition must remain normalized between zero and one.");
        return false;
    }

    if (Instance.MagazineState.Capacity <= 0)
    {
        OutFailureReason = TEXT("Weapon instance magazine capacity must be positive.");
        return false;
    }

    if (Instance.MagazineState.RoundsInMagazine < 0
        || Instance.MagazineState.RoundsInMagazine > Instance.MagazineState.Capacity)
    {
        OutFailureReason = TEXT("Weapon instance magazine round count is outside its valid capacity.");
        return false;
    }

    if (Instance.AmmoReserve < 0)
    {
        OutFailureReason = TEXT("Weapon instance reserve ammunition cannot be negative.");
        return false;
    }

    const bool bCarriesAmmunition = Instance.MagazineState.bRoundChambered
        || Instance.MagazineState.RoundsInMagazine > 0
        || Instance.AmmoReserve > 0;
    if (bCarriesAmmunition && Instance.Build.SelectedAmmoId.IsNone())
    {
        OutFailureReason = TEXT("Weapon instance carries ammunition but its build has no selected ammunition identity.");
        return false;
    }

    FTUResolvedWeaponBuild ResolvedBuild;
    if (!Catalog->ResolveWeaponBuild(Instance.Build, ResolvedBuild, OutFailureReason))
    {
        return false;
    }

    return true;
}
