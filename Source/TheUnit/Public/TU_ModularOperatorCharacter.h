#pragma once

#include "CoreMinimal.h"
#include "TUArmorProtectionComponent.h"
#include "TU_OperatorCharacter.h"
#include "TU_ModularOperatorCharacter.generated.h"

class UTUHealthComponent;
class UTUOperatorAppearanceData;
class UTUOperatorEquipmentComponent;

/**
 * Player/operator character with data-driven body appearance and modular tactical gear.
 * Kept as a subclass so the base movement/combat/FPV character can evolve independently.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_ModularOperatorCharacter : public ATU_OperatorCharacter
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

    UFUNCTION(BlueprintPure, Category="Operator|Armor")
    UTUArmorProtectionComponent* GetArmorProtection() const { return ArmorProtectionComponent; }

    UFUNCTION(BlueprintPure, Category="Operator|Health")
    UTUHealthComponent* GetOperatorHealth() const { return HealthComponent; }

    /** Resolves armor first, then applies the remaining damage to regional health. */
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Armor")
    TObjectPtr<UTUArmorProtectionComponent> ArmorProtectionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Health")
    TObjectPtr<UTUHealthComponent> HealthComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Operator|Appearance")
    TObjectPtr<UTUOperatorAppearanceData> OperatorAppearance = nullptr;
};
