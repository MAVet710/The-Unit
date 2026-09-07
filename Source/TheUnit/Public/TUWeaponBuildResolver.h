#pragma once

#include "CoreMinimal.h"
#include "TheUnitTypes.h"

/** Runtime cache produced from persistent build identity plus immutable definition catalogs. */
struct THEUNIT_API FTUResolvedWeaponBuild
{
    FWeaponDefinition DerivedWeaponDefinition;
    TArray<FWeaponPartDefinition> InstalledPartDefinitions;
    bool bHasFireControl = false;
    FFireControlModuleDefinition FireControlDefinition;
    bool bHasAmmoDefinition = false;
    FAmmoDefinition AmmoDefinition;
};

/** Resolves and validates a saved modular weapon build without owning persistent state. */
struct THEUNIT_API FTUWeaponBuildResolver
{
    static bool ResolveBuild(
        const FWeaponPlatformDefinition& Platform,
        const FWeaponDefinition& BaseWeaponDefinition,
        const FWeaponBuildState& BuildState,
        const TArray<FWeaponPartDefinition>& PartCatalog,
        const TArray<FFireControlModuleDefinition>& FireControlCatalog,
        FTUResolvedWeaponBuild& OutResolvedBuild,
        FString& OutFailureReason);
};
