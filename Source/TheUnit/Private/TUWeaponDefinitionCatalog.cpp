#include "TUWeaponDefinitionCatalog.h"
#include "TUWeaponBuildResolver.h"

namespace
{
template <typename TDefinition, typename TPredicate>
const TDefinition* FindDefinition(const TArray<TDefinition>& Definitions, TPredicate&& Predicate)
{
    return Definitions.FindByPredicate(Forward<TPredicate>(Predicate));
}
}

bool UTUWeaponDefinitionCatalog::GetPlatformDefinition(
    FName PlatformId,
    FWeaponPlatformDefinition& OutDefinition) const
{
    const FWeaponPlatformDefinition* Found = FindDefinition(
        Platforms, [PlatformId](const FWeaponPlatformDefinition& Definition)
        {
            return Definition.PlatformId == PlatformId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetBaseWeaponDefinition(
    FName WeaponId,
    FWeaponDefinition& OutDefinition) const
{
    const FWeaponDefinition* Found = FindDefinition(
        BaseWeaponDefinitions, [WeaponId](const FWeaponDefinition& Definition)
        {
            return Definition.WeaponId == WeaponId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetPartDefinition(
    FName PartId,
    FWeaponPartDefinition& OutDefinition) const
{
    const FWeaponPartDefinition* Found = FindDefinition(
        Parts, [PartId](const FWeaponPartDefinition& Definition)
        {
            return Definition.PartId == PartId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetFireControlDefinition(
    FName FireControlId,
    FFireControlModuleDefinition& OutDefinition) const
{
    const FFireControlModuleDefinition* Found = FindDefinition(
        FireControlModules, [FireControlId](const FFireControlModuleDefinition& Definition)
        {
            return Definition.FireControlId == FireControlId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetAmmoDefinition(
    FName AmmoId,
    FAmmoDefinition& OutDefinition) const
{
    const FAmmoDefinition* Found = FindDefinition(
        Ammunition, [AmmoId](const FAmmoDefinition& Definition)
        {
            return Definition.AmmoId == AmmoId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::ResolveWeaponBuild(
    const FWeaponBuildState& BuildState,
    FTUResolvedWeaponBuild& OutResolvedBuild,
    FString& OutFailureReason) const
{
    OutResolvedBuild = FTUResolvedWeaponBuild();
    OutFailureReason.Reset();

    FWeaponPlatformDefinition Platform;
    if (!GetPlatformDefinition(BuildState.PlatformId, Platform))
    {
        OutFailureReason = TEXT("Weapon build references a platform missing from the definition catalog.");
        return false;
    }

    if (Platform.BaseWeaponDefinitionId.IsNone())
    {
        OutFailureReason = TEXT("Weapon platform does not reference a base weapon definition.");
        return false;
    }

    FWeaponDefinition BaseDefinition;
    if (!GetBaseWeaponDefinition(Platform.BaseWeaponDefinitionId, BaseDefinition))
    {
        OutFailureReason = TEXT("Weapon platform references a base weapon definition missing from the catalog.");
        return false;
    }

    FAmmoDefinition SelectedAmmoDefinition;
    const bool bHasSelectedAmmo = !BuildState.SelectedAmmoId.IsNone();
    if (bHasSelectedAmmo && !GetAmmoDefinition(BuildState.SelectedAmmoId, SelectedAmmoDefinition))
    {
        OutFailureReason = TEXT("Weapon build references ammunition missing from the definition catalog.");
        return false;
    }

    FTUResolvedWeaponBuild ResolvedBuild;
    if (!FTUWeaponBuildResolver::ResolveBuild(
        Platform,
        BaseDefinition,
        BuildState,
        Parts,
        FireControlModules,
        ResolvedBuild,
        OutFailureReason))
    {
        return false;
    }

    if (bHasSelectedAmmo)
    {
        ResolvedBuild.bHasAmmoDefinition = true;
        ResolvedBuild.AmmoDefinition = SelectedAmmoDefinition;
    }

    OutResolvedBuild = MoveTemp(ResolvedBuild);
    return true;
}
