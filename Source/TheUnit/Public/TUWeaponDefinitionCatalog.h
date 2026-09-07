#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TheUnitTypes.h"
#include "TUWeaponDefinitionCatalog.generated.h"

struct FTUResolvedWeaponBuild;

/**
 * Immutable authoring catalog for modular weapon definitions.
 * Runtime weapon state remains on weapon/inventory systems; this object only supplies definitions.
 */
UCLASS(BlueprintType)
class THEUNIT_API UTUWeaponDefinitionCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Definitions")
    TArray<FWeaponPlatformDefinition> Platforms;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Definitions")
    TArray<FWeaponDefinition> BaseWeaponDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Definitions")
    TArray<FWeaponPartDefinition> Parts;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Definitions")
    TArray<FFireControlModuleDefinition> FireControlModules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Definitions")
    TArray<FTriggerDefinition> Triggers;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Definitions")
    TArray<FAmmoDefinition> Ammunition;

    UFUNCTION(BlueprintPure, Category = "Weapon|Definitions")
    bool GetPlatformDefinition(FName PlatformId, FWeaponPlatformDefinition& OutDefinition) const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Definitions")
    bool GetBaseWeaponDefinition(FName WeaponId, FWeaponDefinition& OutDefinition) const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Definitions")
    bool GetPartDefinition(FName PartId, FWeaponPartDefinition& OutDefinition) const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Definitions")
    bool GetFireControlDefinition(FName FireControlId, FFireControlModuleDefinition& OutDefinition) const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Definitions")
    bool GetTriggerDefinition(FName TriggerId, FTriggerDefinition& OutDefinition) const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Definitions")
    bool GetAmmoDefinition(FName AmmoId, FAmmoDefinition& OutDefinition) const;

    /** C++ resolution path; the resolved cache intentionally is not persistent/Blueprint-owned. */
    bool ResolveWeaponBuild(
        const FWeaponBuildState& BuildState,
        FTUResolvedWeaponBuild& OutResolvedBuild,
        FString& OutFailureReason) const;
};
