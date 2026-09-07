#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheUnitTypes.h"
#include "TU_WeaponBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTUWeaponAttachmentComponent;
class UTUWeaponLoadoutData;
class UTUWeaponComponent;

UENUM(BlueprintType)
enum class ETUFireMode : uint8
{
    SemiAuto UMETA(DisplayName = "Semi Auto"),
    Burst UMETA(DisplayName = "Burst"),
    FullAuto UMETA(DisplayName = "Full Auto")
};

USTRUCT(BlueprintType)
struct FTUWeaponShotResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bFired = false;

    UPROPERTY(BlueprintReadOnly)
    bool bHit = false;

    UPROPERTY(BlueprintReadOnly)
    FVector TraceStart = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector TraceEnd = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> HitActor = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTUOnWeaponShot, FTUWeaponShotResult, ShotResult);

/**
 * Canonical runtime weapon API. Owns action routing, cadence, hitscan/recoil and reload lifecycle;
 * its private mechanics component owns definitions and all ammunition mutations.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_WeaponBase : public AActor
{
    GENERATED_BODY()

public:
    ATU_WeaponBase();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetAiming(bool bNewAiming) { bIsAiming = bNewAiming; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool IsAiming() const { return bIsAiming; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool IsReloading() const { return bIsReloading; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    float GetFireIntervalSeconds() const;

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

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    UStaticMeshComponent* GetWeaponBodyMesh() const { return WeaponBodyMesh; }

    UFUNCTION(BlueprintPure, Category = "Weapon|Attachments")
    UTUWeaponAttachmentComponent* GetAttachmentComponent() const { return AttachmentComponent; }

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FTUOnWeaponShot OnShotFired;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Visual")
    TObjectPtr<USceneComponent> WeaponRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Visual")
    TObjectPtr<UStaticMeshComponent> WeaponBodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Attachments")
    TObjectPtr<UTUWeaponAttachmentComponent> AttachmentComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
    TObjectPtr<UTUWeaponLoadoutData> DefaultAttachmentLoadout = nullptr;

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

    /** New runtime rifles use timed cadence; false preserves the original deterministic test skeleton. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Runtime")
    bool bUseTimedFireCadence = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Runtime", meta=(ClampMin="0.0"))
    float ReloadDurationSeconds = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Runtime", meta=(ClampMin="100.0"))
    float TraceRangeCm = 100000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Runtime")
    FName MuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Runtime")
    bool bIsAiming = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Runtime")
    FTUWeaponShotResult LastShotResult;

    void ConfigureWeaponDefaults(
        const FWeaponDefinition& WeaponDefinition,
        const FAmmoDefinition& AmmoDefinition,
        const FMagazineState& MagazineState,
        int32 ReserveAmmo);

private:
    void PerformHitscanShot();
    void ApplyRecoil();
    void ScheduleNextBurstShot();
    void ScheduleNextFullAutoShot();

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<UTUWeaponComponent> WeaponMechanics;

    FTimerHandle FireTimerHandle;
    FTimerHandle ReloadTimerHandle;
    float NextAllowedFireTimeSeconds = 0.0f;
};
