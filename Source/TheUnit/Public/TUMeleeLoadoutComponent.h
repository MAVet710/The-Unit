#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUMeleeLoadoutComponent.generated.h"

class ATU_OTFKnife;

/** One selectable item for the operator's single melee equipment slot. */
USTRUCT(BlueprintType)
struct FTUMeleeEquipmentEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout")
    TSubclassOf<ATU_OTFKnife> MeleeClass;

    /** Socket used when this melee item is drawn in first person. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout")
    FName EquipSocket = TEXT("weapon_socket");

    /** Gameplay inventory weight; independent of the external art asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout", meta=(ClampMin="0.0"))
    float WeightKg = 0.0f;

    bool IsValidEntry() const
    {
        return !ItemId.IsNone() && MeleeClass != nullptr;
    }
};

/**
 * Owns the operator's melee equipment slot and available melee choices.
 * The component stores selection only; the operator owns/spawns the runtime actor.
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUMeleeLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUMeleeLoadoutComponent();

    UFUNCTION(BlueprintCallable, Category="Melee Loadout")
    bool AddOrReplaceItem(const FTUMeleeEquipmentEntry& Entry);

    UFUNCTION(BlueprintCallable, Category="Melee Loadout")
    bool RemoveItem(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Melee Loadout")
    bool SelectItemById(FName ItemId);

    /** Cycle through available choices. Positive moves forward, negative backward. */
    UFUNCTION(BlueprintCallable, Category="Melee Loadout")
    bool CycleSelection(int32 Direction = 1);

    /** Replace the available list, preserving PreferredItemId when possible. */
    UFUNCTION(BlueprintCallable, Category="Melee Loadout")
    void SetItems(const TArray<FTUMeleeEquipmentEntry>& NewItems, FName PreferredItemId = NAME_None);

    UFUNCTION(BlueprintPure, Category="Melee Loadout")
    bool GetSelectedItem(FTUMeleeEquipmentEntry& OutItem) const;

    UFUNCTION(BlueprintPure, Category="Melee Loadout")
    FName GetSelectedItemId() const { return SelectedItemId; }

    UFUNCTION(BlueprintPure, Category="Melee Loadout")
    TSubclassOf<ATU_OTFKnife> GetSelectedMeleeClass() const;

    UFUNCTION(BlueprintPure, Category="Melee Loadout")
    float GetSelectedWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Melee Loadout")
    TArray<FTUMeleeEquipmentEntry> GetAvailableItems() const { return AvailableItems; }

protected:
    /** Editable inventory exposed to a future armory/loadout UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout")
    TArray<FTUMeleeEquipmentEntry> AvailableItems;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Loadout")
    FName SelectedItemId = NAME_None;

private:
    int32 FindItemIndex(FName ItemId) const;
    void EnsureValidSelection();
};
