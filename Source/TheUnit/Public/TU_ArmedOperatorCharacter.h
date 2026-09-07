#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "TU_OperatorCharacter.h"
#include "TUOperatorLoadoutComponent.h"
#include "TUArmoryWidget.h"
#include "TU_ArmedOperatorCharacter.generated.h"

class ATU_OTFKnife;
class ATU_WeaponBase;
class UTUArmoryWidget;
class UTUBriefingWidget;
class UTUMeleeLoadoutComponent;

/** Operator layer that owns primary, secondary, selectable melee and command-center station UI. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_ArmedOperatorCharacter : public ATU_OperatorCharacter
{
    GENERATED_BODY()

public:
    ATU_ArmedOperatorCharacter();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Interact() override;

    UFUNCTION(BlueprintPure, Category="Weapon")
    ATU_WeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

    UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
    ATU_WeaponBase* GetPrimaryWeapon() const { return PrimaryWeapon; }

    UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
    ATU_WeaponBase* GetSecondaryWeapon() const { return SecondaryWeapon; }

    UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
    ETUOperatorWeaponSlot GetActiveWeaponSlot() const { return ActiveWeaponSlot; }

    UFUNCTION(BlueprintPure, Category="Weapon|Loadout")
    UTUOperatorLoadoutComponent* GetOperatorLoadout() const { return OperatorLoadout; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool SpawnDefaultWeapon();

    UFUNCTION(BlueprintCallable, Category="Weapon|Loadout")
    bool EquipWeaponSlot(ETUOperatorWeaponSlot Slot);

    UFUNCTION(BlueprintCallable, Category="Weapon|Loadout")
    bool SelectPrimaryById(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Weapon|Loadout")
    bool SelectSecondaryById(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Equipment|Loadout")
    bool SelectEquipmentById(FName ItemId);

    UFUNCTION(BlueprintPure, Category="Loadout")
    float GetSelectedLoadoutWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Melee")
    ATU_OTFKnife* GetCurrentMelee() const { return CurrentMelee; }

    UFUNCTION(BlueprintPure, Category="Melee|Loadout")
    UTUMeleeLoadoutComponent* GetMeleeLoadout() const { return MeleeLoadout; }

    UFUNCTION(BlueprintPure, Category="Melee|Loadout")
    FName GetSelectedMeleeId() const;

    UFUNCTION(BlueprintPure, Category="Melee|Loadout")
    float GetSelectedMeleeWeightKg() const;

    UFUNCTION(BlueprintCallable, Category="Melee")
    bool SpawnDefaultMelee();

    UFUNCTION(BlueprintCallable, Category="Melee|Loadout")
    bool SelectMeleeById(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Melee|Loadout")
    bool CycleMeleeSelection(int32 Direction = 1);

    UFUNCTION(BlueprintCallable, Category="Melee")
    bool DrawMelee();

    UFUNCTION(BlueprintCallable, Category="Melee")
    bool HolsterMelee();

    UFUNCTION(BlueprintPure, Category="Melee")
    bool IsMeleeEquipped() const { return bMeleeEquipped; }

    UFUNCTION(BlueprintCallable, Category="Armory")
    bool OpenArmory();

    UFUNCTION(BlueprintCallable, Category="Armory")
    bool OpenArmoryView(ETUArmoryViewMode ViewMode);

    UFUNCTION(BlueprintCallable, Category="Armory")
    void CloseArmory();

    UFUNCTION(BlueprintCallable, Category="Armory")
    void ToggleArmory();

    UFUNCTION(BlueprintPure, Category="Armory")
    bool IsArmoryOpen() const { return IsValid(ArmoryWidget); }

    UFUNCTION(BlueprintCallable, Category="Briefing")
    bool OpenBriefing(FName MissionId, const FText& MissionTitle);

    UFUNCTION(BlueprintCallable, Category="Briefing")
    void CloseBriefing();

    UFUNCTION(BlueprintPure, Category="Briefing")
    bool IsBriefingOpen() const { return IsValid(BriefingWidget); }

    UFUNCTION(BlueprintPure, Category="Command Center")
    bool IsCommandCenterUIOpen() const { return IsArmoryOpen() || IsBriefingOpen(); }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    TSubclassOf<ATU_WeaponBase> DefaultWeaponClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FName FirstPersonWeaponSocket = TEXT("weapon_socket");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Loadout")
    TObjectPtr<UTUOperatorLoadoutComponent> OperatorLoadout;

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|Loadout")
    TObjectPtr<ATU_WeaponBase> PrimaryWeapon = nullptr;

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|Loadout")
    TObjectPtr<ATU_WeaponBase> SecondaryWeapon = nullptr;

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<ATU_WeaponBase> CurrentWeapon = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|Loadout")
    ETUOperatorWeaponSlot ActiveWeaponSlot = ETUOperatorWeaponSlot::Primary;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Loadout")
    TObjectPtr<UTUMeleeLoadoutComponent> MeleeLoadout;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
    TSubclassOf<ATU_OTFKnife> DefaultMeleeClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
    FName FirstPersonMeleeSocket = TEXT("weapon_socket");

    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Melee")
    TObjectPtr<ATU_OTFKnife> CurrentMelee = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Melee")
    bool bMeleeEquipped = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Melee")
    bool bMeleeHolstering = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Command Center|Armory")
    TSubclassOf<UTUArmoryWidget> ArmoryWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UTUArmoryWidget> ArmoryWidget = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Command Center|Briefing")
    TSubclassOf<UTUBriefingWidget> BriefingWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UTUBriefingWidget> BriefingWidget = nullptr;

    /** Developer escape hatch. Production command-center flow accesses armory physically with F. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Command Center|Debug")
    bool bAllowPortableArmoryDebug = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Command Center|Interaction", meta=(ClampMin="100.0", ClampMax="1000.0"))
    float CommandCenterInteractRangeCm = 400.0f;

private:
    ATU_WeaponBase* SpawnWeaponClass(TSubclassOf<ATU_WeaponBase> WeaponClass, bool bVisible);
    bool EnsureWeaponSlotSpawned(ETUOperatorWeaponSlot Slot);
    bool ReplaceWeaponSlot(ETUOperatorWeaponSlot Slot);
    void DestroyLoadoutWeapons();
    void RestoreGameInputMode();

    void StartWeaponFire();
    void StopWeaponFire();
    void ReloadWeapon();
    void CycleWeaponFireMode();
    void StartWeaponADS();
    void StopWeaponADS();

    void EquipPrimaryInput();
    void EquipSecondaryInput();
    void ToggleMelee();
    void CycleMeleeInput();
    void ToggleArmoryInput();
    void FinishMeleeHolster();
    void DestroyCurrentMelee();

    FName CurrentMeleeSocket = TEXT("weapon_socket");
    FTimerHandle MeleeHolsterTimerHandle;
};
