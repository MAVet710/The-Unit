#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheUnitTypes.h"
#include "TUEquipmentTypes.h"
#include "TUArmorProtectionComponent.generated.h"

class UTUEquipmentDefinition;
class UTUOperatorEquipmentComponent;

USTRUCT(BlueprintType)
struct FTUArmorHitResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    float IncomingDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    float FinalDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    float DurabilityBefore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    float DurabilityAfter = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    float ArmorDamageApplied = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    FName ArmorItemId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    ETUEquipmentSlot ArmorSlot = ETUEquipmentSlot::Accessory;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    bool bArmorPresent = false;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    bool bCoverageHit = false;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    bool bStopped = false;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    bool bPenetrated = false;

    UPROPERTY(BlueprintReadOnly, Category="Armor")
    bool bArmorDepleted = false;
};

USTRUCT()
struct FTUArmorRuntimeState
{
    GENERATED_BODY()

    UPROPERTY()
    FName ItemKey = NAME_None;

    UPROPERTY()
    float Durability = 0.0f;
};

/**
 * Resolves ballistic hits against currently equipped protective gear.
 * Durability is stored per operator instance rather than on shared data assets.
 */
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUArmorProtectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUArmorProtectionComponent();

    void InitializeEquipment(UTUOperatorEquipmentComponent* InEquipmentComponent);

    UFUNCTION(BlueprintCallable, Category="Operator|Armor")
    FTUArmorHitResult ResolveBallisticHit(
        ETUBodyRegion Region,
        float IncomingDamage,
        float Penetration,
        float ArmorDamage,
        float CoverageRoll01);

    UFUNCTION(BlueprintPure, Category="Operator|Armor")
    float GetArmorDurability(ETUEquipmentSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category="Operator|Armor")
    void ResetArmorState();

private:
    const UTUEquipmentDefinition* FindBestProtectiveItem(
        ETUBodyRegion Region,
        float CoverageRoll01,
        ETUEquipmentSlot& OutSlot) const;

    FName MakeItemKey(const UTUEquipmentDefinition* Definition) const;
    FTUArmorRuntimeState& GetOrCreateRuntimeState(ETUEquipmentSlot Slot, const UTUEquipmentDefinition* Definition);

    UPROPERTY(Transient)
    TObjectPtr<UTUOperatorEquipmentComponent> EquipmentComponent = nullptr;

    UPROPERTY(Transient)
    TMap<ETUEquipmentSlot, FTUArmorRuntimeState> RuntimeArmorState;
};
