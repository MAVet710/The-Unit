#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "TU_OperatorCharacter.h"
#include "TU_ArmedOperatorCharacter.generated.h"

class ATU_OTFKnife;
class ATU_WeaponBase;
class UTUMeleeLoadoutComponent;

/** Operator layer that owns a first-person firearm plus a selectable melee slot. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_ArmedOperatorCharacter : public ATU_OperatorCharacter
{
    GENERATED_BODY()

public:
    ATU_ArmedOperatorCharacter();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="Weapon")
    ATU_WeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool SpawnDefaultWeapon();

    UFUNCTION(BlueprintPure, Category="Melee")
    ATU_OTFKnife* GetCurrentMelee() const { return CurrentMelee; }

    UFUNCTION(BlueprintPure, Category="Melee|Loadout")
    UTUMeleeLoadoutComponent* GetMeleeLoadout() const { return MeleeLoadout; }

    UFUNCTION(BlueprintPure, Category="Melee|Loadout")
    FName GetSelectedMeleeId() const;

    UFUNCTION(BlueprintCallable, Category="Melee")
    bool SpawnDefaultMelee();

    /** Select a melee item by loadout id while the melee slot is holstered. */
    UFUNCTION(BlueprintCallable, Category="Melee|Loadout")
    bool SelectMeleeById(FName ItemId);

    /** Cycle the selected melee item while holstered. */
    UFUNCTION(BlueprintCallable, Category="Melee|Loadout")
    bool CycleMeleeSelection(int32 Direction = 1);

    /** Hide the firearm, attach the selected melee item, and begin its draw/readiness presentation. */
    UFUNCTION(BlueprintCallable, Category="Melee")
    bool DrawMelee();

    /** Finish the melee item's holster presentation, then restore the firearm. */
    UFUNCTION(BlueprintCallable, Category="Melee")
    bool HolsterMelee();

    UFUNCTION(BlueprintPure, Category="Melee")
    bool IsMeleeEquipped() const { return bMeleeEquipped; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<ATU_WeaponBase> DefaultWeaponClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FName FirstPersonWeaponSocket = TEXT("weapon_socket");

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<ATU_WeaponBase> CurrentWeapon = nullptr;

    /** Single melee equipment slot. Contains the built-in OTF and karambit choices by default. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Loadout")
    TObjectPtr<UTUMeleeLoadoutComponent> MeleeLoadout;

    /** Backward-compatible fallback used only when the loadout has no valid selected item. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
    TSubclassOf<ATU_OTFKnife> DefaultMeleeClass;

    /** Fallback socket for legacy melee classes or empty loadouts. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
    FName FirstPersonMeleeSocket = TEXT("weapon_socket");

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Melee")
    TObjectPtr<ATU_OTFKnife> CurrentMelee = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Melee")
    bool bMeleeEquipped = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Melee")
    bool bMeleeHolstering = false;

private:
    void StartWeaponFire();
    void StopWeaponFire();
    void ReloadWeapon();
    void CycleWeaponFireMode();
    void StartWeaponADS();
    void StopWeaponADS();

    void ToggleMelee();
    void CycleMeleeInput();
    void FinishMeleeHolster();
    void DestroyCurrentMelee();

    FName CurrentMeleeSocket = TEXT("weapon_socket");
    FTimerHandle MeleeHolsterTimerHandle;
};
