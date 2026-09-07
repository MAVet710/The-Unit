#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUWeaponAttachmentDefinition.h"
#include "TUWeaponAttachmentComponent.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTUWeaponLoadoutData;

/** Owns weapon attachment slot state, visuals and aggregate handling modifiers. */
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUWeaponAttachmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUWeaponAttachmentComponent();

    void InitializeVisualRoot(USceneComponent* InVisualRoot);

    UFUNCTION(BlueprintCallable, Category="Weapon|Attachments")
    bool EquipAttachment(UTUWeaponAttachmentDefinition* Definition);

    UFUNCTION(BlueprintCallable, Category="Weapon|Attachments")
    bool UnequipAttachment(ETUWeaponAttachmentSlot Slot);

    UFUNCTION(BlueprintCallable, Category="Weapon|Attachments")
    void ClearAttachments();

    UFUNCTION(BlueprintCallable, Category="Weapon|Attachments")
    bool ApplyLoadout(UTUWeaponLoadoutData* Loadout);

    UFUNCTION(BlueprintPure, Category="Weapon|Attachments")
    UTUWeaponAttachmentDefinition* GetAttachment(ETUWeaponAttachmentSlot Slot) const;

    UFUNCTION(BlueprintPure, Category="Weapon|Attachments")
    float GetRecoilMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Weapon|Attachments")
    float GetSpreadMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Weapon|Attachments")
    float GetAttachmentWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Weapon|Attachments")
    UStaticMeshComponent* GetAttachmentVisual(ETUWeaponAttachmentSlot Slot) const;

private:
    void RebuildVisual(ETUWeaponAttachmentSlot Slot);
    void DestroyVisual(ETUWeaponAttachmentSlot Slot);

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> VisualRoot = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|Attachments", meta=(AllowPrivateAccess="true"))
    TMap<ETUWeaponAttachmentSlot, TObjectPtr<UTUWeaponAttachmentDefinition>> EquippedAttachments;

    UPROPERTY(Transient)
    TMap<ETUWeaponAttachmentSlot, TObjectPtr<UStaticMeshComponent>> AttachmentVisuals;
};
