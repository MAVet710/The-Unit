#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_WeaponBase.generated.h"

UENUM(BlueprintType)
enum class ETUFireMode : uint8
{
    SemiAuto UMETA(DisplayName = "Semi Auto"),
    Burst UMETA(DisplayName = "Burst"),
    FullAuto UMETA(DisplayName = "Full Auto")
};

/**
 * Base weapon actor.
 * Phase 1C implements a compile-safe ammo, reload, and fire mode skeleton.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_WeaponBase : public AActor
{
    GENERATED_BODY()

public:
    ATU_WeaponBase();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Fire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void FireSingleShot();

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void HandleBurstFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void HandleFullAutoFire();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool CanFire() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void FinishReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void AddReserveAmmo(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetCurrentAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetReserveAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|FireMode")
    ETUFireMode GetCurrentFireMode() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void SetFireMode(ETUFireMode NewFireMode);

    UFUNCTION(BlueprintCallable, Category = "Weapon|FireMode")
    void CycleFireMode();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FText WeaponDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
    int32 MagazineCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 CurrentAmmoInMagazine;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 ReserveAmmo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.01"))
    float FireRate;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
    float Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    bool bCanFire;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bIsReloading;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FireMode")
    TArray<ETUFireMode> AvailableFireModes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|FireMode")
    ETUFireMode CurrentFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FireMode", meta = (ClampMin = "1"))
    int32 BurstCount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|FireMode", meta = (ClampMin = "0"))
    int32 ShotsRemainingInBurst;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FireMode", meta = (ClampMin = "0.01"))
    float TimeBetweenShots;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|FireMode")
    bool bIsFiring;
};
