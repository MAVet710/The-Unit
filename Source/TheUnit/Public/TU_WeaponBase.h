#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_WeaponBase.generated.h"

/**
 * Base weapon actor.
 * Phase 1C implements a simple compile-safe ammo and reload skeleton.
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
    void StartReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void FinishReload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void AddReserveAmmo(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetCurrentAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetReserveAmmo() const;

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
};
