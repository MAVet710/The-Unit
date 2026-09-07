#pragma once

#include "CoreMinimal.h"
#include "TheUnitTypes.h"

/** Pure compatibility/stat rules for modular weapon builds. Owns no mutable weapon state. */
struct THEUNIT_API FTUWeaponBuildRules
{
    static TArray<FName> GatherAvailableInterfaceTags(
        const FWeaponPlatformDefinition& Platform,
        const TArray<FWeaponPartDefinition>& InstalledParts);

    static bool AreRequiredTagsSatisfied(
        const TArray<FName>& AvailableTags,
        const TArray<FName>& RequiredTags);

    static bool IsPartCompatible(
        const FWeaponPlatformDefinition& Platform,
        const FWeaponPartDefinition& CandidatePart,
        const TArray<FWeaponPartDefinition>& InstalledParts,
        FString* OutFailureReason = nullptr);

    static bool IsAmmoCompatible(
        const FWeaponPlatformDefinition& Platform,
        FName AmmoId);

    static FWeaponDefinition DeriveWeaponDefinition(
        const FWeaponDefinition& BaseDefinition,
        const TArray<FWeaponPartDefinition>& InstalledParts);
};
