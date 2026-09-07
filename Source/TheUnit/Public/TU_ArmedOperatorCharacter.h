#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "TU_OperatorCharacter.h"
#include "TU_ArmedOperatorCharacter.generated.h"

class ATU_OTFKnife;
class ATU_WeaponBase;

/** Operator layer that owns a first-person firearm plus a quick-draw melee item. */
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

    UFUNCTION(BlueprintCallable, Category="Melee")
    bool SpawnDefaultMelee();

    /** Hide the firearm, attach the knife to the first-person hand, and deploy the blade. */
    UFUNCTION(BlueprintCallable, Category="Melee")
    bool DrawMelee();

    /** Retract the blade, then restore the firearm after the presentation finishes. */
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
    TSubclassOf<ATU_OTFKnife> DefaultMeleeClass;

    /** Uses the weapon socket by default so current prototype arms need no extra socket. */
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
    void FinishMeleeHolster();

    FTimerHandle MeleeHolsterTimerHandle;
};
