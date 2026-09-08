#pragma once

#include "CoreMinimal.h"
#include "TUArmorProtectionComponent.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_ModularOperatorCharacter.generated.h"

class UTUEquipmentDefinition;
class UTUHealthComponent;
class UTUOperatorAppearanceData;
class UTUOperatorEquipmentComponent;

/**
 * Full operator integration layer: armed operator gameplay plus data-driven body,
 * removable tactical equipment, ballistic protection and regional health.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_ModularOperatorCharacter : public ATU_ArmedOperatorCharacter
{
    GENERATED_BODY()

public:
    ATU_ModularOperatorCharacter();

    UFUNCTION(BlueprintCallable, Category="Operator|Appearance")
    void SetOperatorAppearance(UTUOperatorAppearanceData* NewAppearance);

    UFUNCTION(BlueprintCallable, Category="Operator|Appearance")
    void ApplyOperatorAppearance();

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    UTUOperatorEquipmentComponent* GetOperatorEquipment() const { return EquipmentComponent; }

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    TArray<UTUEquipmentDefinition*> GetAvailableGear() const;

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    void SetAvailableGear(const TArray<UTUEquipmentDefinition*>& NewGear);

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    bool EquipGearById(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    bool UnequipGearSlot(ETUEquipmentSlot Slot);

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    FName GetEquippedGearId(ETUEquipmentSlot Slot) const;

    UFUNCTION(BlueprintPure, Category="Operator|Armor")
    UTUArmorProtectionComponent* GetArmorProtection() const { return ArmorProtectionComponent; }

    UFUNCTION(BlueprintPure, Category="Operator|Health")
    UTUHealthComponent* GetOperatorHealth() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category="Operator|Loadout")
    float GetTotalCombatLoadoutWeightKg() const;

    /** Resolves equipped armor first, then applies remaining damage to regional health. */
    UFUNCTION(BlueprintCallable, Category="Operator|Damage")
    FTUArmorHitResult ApplyBallisticRegionalDamage(
        ETUBodyRegion Region,
        float IncomingDamage,
        float Penetration,
        float ArmorDamage,
        float CoverageRoll01);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Equipment")
    TObjectPtr<UTUOperatorEquipmentComponent> EquipmentComponent;

    /** Data-authored inventory shown by the Cage. Appearance default-loadout items are automatically included. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Operator|Equipment")
    TArray<TObjectPtr<UTUEquipmentDefinition>> AvailableGear;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Armor")
    TObjectPtr<UTUArmorProtectionComponent> ArmorProtectionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Health")
    TObjectPtr<UTUHealthComponent> HealthComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Operator|Appearance")
    TObjectPtr<UTUOperatorAppearanceData> OperatorAppearance = nullptr;

private:
    UTUEquipmentDefinition* FindAvailableGear(FName ItemId) const;
    void AddAppearanceLoadoutToCatalog();
};
