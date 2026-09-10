#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUEquipmentTypes.h"
#include "TUOperatorEquipmentComponent.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UTUEquipmentDefinition;
class UTUOperatorLoadoutData;

/**
 * Owns logical operator equipment state and creates the matching visual components.
 * Weighted skeletal gear can follow the body pose while rigid gear attaches to sockets.
 */
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUOperatorEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUOperatorEquipmentComponent();

    /** Supplies the body and first-person arm meshes that equipment visuals attach to. */
    void InitializeVisualRoots(USkeletalMeshComponent* InThirdPersonBody, USkeletalMeshComponent* InFirstPersonArms);

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    bool EquipItem(UTUEquipmentDefinition* Definition);

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    bool UnequipSlot(ETUEquipmentSlot Slot);

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    void ClearLoadout();

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    bool ApplyLoadout(UTUOperatorLoadoutData* Loadout);

    UFUNCTION(BlueprintCallable, Category="Operator|Equipment")
    void RebuildVisuals();

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    UTUEquipmentDefinition* GetEquippedItem(ETUEquipmentSlot Slot) const;

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    bool IsSlotOccupied(ETUEquipmentSlot Slot) const;

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    float GetTotalEquipmentWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Operator|Equipment")
    int32 GetEquippedItemCount() const { return EquippedItems.Num(); }

private:
    USceneComponent* CreateVisualForDefinition(const UTUEquipmentDefinition* Definition, bool bFirstPerson);
    void DestroyVisualForSlot(ETUEquipmentSlot Slot);
    void DestroyAllVisuals();
    bool AreSkeletonsCompatible(const USkeletalMeshComponent* Leader, const USkeletalMeshComponent* Follower) const;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> ThirdPersonBody = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> FirstPersonArms = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Operator|Equipment", meta=(AllowPrivateAccess="true"))
    TMap<ETUEquipmentSlot, TObjectPtr<UTUEquipmentDefinition>> EquippedItems;

    UPROPERTY(Transient)
    TMap<ETUEquipmentSlot, TObjectPtr<USceneComponent>> ThirdPersonVisuals;

    UPROPERTY(Transient)
    TMap<ETUEquipmentSlot, TObjectPtr<USceneComponent>> FirstPersonVisuals;
};
