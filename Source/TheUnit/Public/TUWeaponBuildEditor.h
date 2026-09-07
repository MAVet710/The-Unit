#pragma once

#include "CoreMinimal.h"
#include "TheUnitTypes.h"

/**
 * Pure build-edit operations for customization/loadout workflows.
 * Mutates only the supplied build identity after the proposed final build validates.
 */
struct THEUNIT_API FTUWeaponBuildEditor
{
    static bool TryInstallPart(
        const FWeaponPlatformDefinition& Platform,
        const FWeaponPartDefinition& CandidatePart,
        const TArray<FWeaponPartDefinition>& PartCatalog,
        FWeaponBuildState& InOutBuild,
        FString& OutFailureReason);

    static bool TryRemovePart(
        const FWeaponPlatformDefinition& Platform,
        FName PartId,
        const TArray<FWeaponPartDefinition>& PartCatalog,
        FWeaponBuildState& InOutBuild,
        FString& OutFailureReason);

    static bool TryReplacePart(
        const FWeaponPlatformDefinition& Platform,
        FName ExistingPartId,
        const FWeaponPartDefinition& ReplacementPart,
        const TArray<FWeaponPartDefinition>& PartCatalog,
        FWeaponBuildState& InOutBuild,
        FString& OutFailureReason);

    static bool TrySelectAmmo(
        const FWeaponPlatformDefinition& Platform,
        FName AmmoId,
        FWeaponBuildState& InOutBuild,
        FString& OutFailureReason);
};
