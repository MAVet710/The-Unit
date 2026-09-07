#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheUnitTypes.h"
#include "TU_WeaponBase.generated.h"

class UTUWeaponComponent;
class UTUWeaponDefinitionCatalog;
struct FTUResolvedWeaponBuild;

/**
 * Canonical runtime weapon API. Owns action routing and reload lifecycle;
 * its private component owns definitions and all ammunition mutations.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_WeaponBase : public AActor
{
    GENERATED_BODY()

public:
    ATU_WeaponBase();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Fire();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool CanFire() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void FireSingleShot();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void HandleBurstFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void HandleFullAutoFire();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    ETUFireMode GetCurrentFireMode() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetFireMode(ETUFireMode NewFireMode);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void CycleFireMode();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void FinishReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void AddReserveAmmo(int32 Amount);

    /** Applies a resolver-produced modular build atomically. C++ only until catalogs/runtime ownership are finalized. */
    bool ApplyResolvedBuild(const FTUResolvedWeaponBuild& ResolvedBuild, FString& OutFailureReason);

    /** Resolves immutable definitions from a catalog and applies them atomically to this runtime weapon. */
    bool ConfigureFromCatalog(
        const UTUWeaponDefinitionCatalog* Catalog,
        const FWeaponBuildState& BuildState,
        FString& OutFailureReason);

    /** Total loaded rounds, including the chamber. */
    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetCurrentAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetReserveAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    FMagazineState GetMagazineState() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    FWeaponDefinition GetWeaponDefinition() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    FAmmoDefinition GetAmmoDefinition() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|FireControl")
    TArray<ETUFireMode> GetAvailableFireModes() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|FireControl")
    bool HasActiveFireControl() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    bool HasActiveTrigger() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    FName GetActiveTriggerId() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    ETUTriggerType GetActiveTriggerType() const;

    /** Legacy-compatible profile accessor; dedicated Trigger parts take precedence. */
    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    FName GetActiveTriggerProfileId() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|FireControl")
    int32 GetConfiguredBurstCount() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    float GetTriggerResponseMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    float GetResetResponseMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    float GetSemiAutoResetDelaySeconds() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Trigger")
    bool RequiresReleaseBetweenSemiShots() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    bool bCanFire;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bIsReloading;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TArray<ETUFireMode> AvailableFireModes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    ETUFireMode CurrentFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
    int32 BurstCount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    int32 ShotsRemainingInBurst;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bIsFiring;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|FireControl")
    bool bHasActiveFireControl = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|FireControl")
    FFireControlModuleDefinition ActiveFireControlDefinition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Trigger")
    bool bHasActiveTrigger = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Trigger")
    FTriggerDefinition ActiveTriggerDefinition;

private:
    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    UTUWeaponComponent* WeaponMechanics;
};
