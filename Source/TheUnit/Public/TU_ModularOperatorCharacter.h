#pragma once

#include "CoreMinimal.h"
#include "TU_OperatorCharacter.h"
#include "TU_ModularOperatorCharacter.generated.h"

class UTUArmorProtectionComponent;
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

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Equipment")
    TObjectPtr<UTUOperatorEquipmentComponent> EquipmentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operator|Armor")
    TObjectPtr<UTUArmorProtectionComponent> ArmorProtectionComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Operator|Appearance")
    TObjectPtr<UTUOperatorAppearanceData> OperatorAppearance = nullptr;
};
