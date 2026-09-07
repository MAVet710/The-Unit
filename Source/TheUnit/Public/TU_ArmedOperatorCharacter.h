#pragma once

#include "CoreMinimal.h"
#include "TU_OperatorCharacter.h"
#include "TU_ArmedOperatorCharacter.generated.h"

class ATU_WeaponBase;

/** Operator layer that owns a first-person runtime weapon without changing the base movement pawn. */
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

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<ATU_WeaponBase> DefaultWeaponClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FName FirstPersonWeaponSocket = TEXT("weapon_socket");

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<ATU_WeaponBase> CurrentWeapon = nullptr;

private:
    void StartWeaponFire();
    void StopWeaponFire();
    void ReloadWeapon();
    void CycleWeaponFireMode();
    void StartWeaponADS();
    void StopWeaponADS();
};
