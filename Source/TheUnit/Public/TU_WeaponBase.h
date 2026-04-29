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
 * Phase 1C.1 provides a compile-safe ammo, reload, and fire mode skeleton.
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

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
    void FireSingleShot();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
    void HandleBurstFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
    void HandleFullAutoFire();

    UFUNCTION(BlueprintPure, Category = "Weapon|Fire Mode")
    ETUFireMode GetCurrentFireMode() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire Mode")
    void SetFireMode(ETUFireMode NewFireMode);

    UFUNCTION(BlueprintCallable, Category = "Weapon|Fire Mode")
    void CycleFireMode();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
    void StartReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
    void FinishReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
    void AddReserveAmmo(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
    int32 GetCurrentAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
    int32 GetReserveAmmo() const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FText WeaponDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "1"))
    int32 MagazineCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "0"))
    int32 CurrentAmmoInMagazine;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "0"))
    int32 ReserveAmmo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.01"))
    float FireRate;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
    float Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|State")
    bool bCanFire;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Reload")
    bool bIsReloading;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire Mode")
    TArray<ETUFireMode> AvailableFireModes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire Mode")
    ETUFireMode CurrentFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire Mode", meta = (ClampMin = "1"))
    int32 BurstCount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Fire Mode")
    int32 ShotsRemainingInBurst;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire Mode", meta = (ClampMin = "0.01"))
    float TimeBetweenShots;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
    bool bIsFiring;
};
