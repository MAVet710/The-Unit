#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUOperatorLoadoutComponent.generated.h"

class ATU_WeaponBase;

UENUM(BlueprintType)
enum class ETUOperatorWeaponSlot : uint8
{
    Primary UMETA(DisplayName="Primary"),
    Secondary UMETA(DisplayName="Secondary")
};

USTRUCT(BlueprintType)
struct FTUOperatorWeaponEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout")
    TSubclassOf<ATU_WeaponBase> WeaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout", meta=(ClampMin="0.0"))
    float WeightKg = 0.0f;

    bool IsValidEntry() const
    {
        return !ItemId.IsNone() && WeaponClass != nullptr;
    }
};

USTRUCT(BlueprintType)
struct FTUOperatorEquipmentEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout")
    FText DisplayName;

    /** Runtime equipment class intentionally deferred until the equipment actor boundary is finalized. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout", meta=(ClampMin="0.0"))
    float WeightKg = 0.0f;

    bool IsValidEntry() const
    {
        return !ItemId.IsNone();
    }
};

/**
 * Data-driven operator loadout for Primary, Secondary and Equipment slots.
 * Melee remains owned by UTUMeleeLoadoutComponent so PR #20 stays the source
 * of truth for OTF/Karambit selection.
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUOperatorLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUOperatorLoadoutComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Operator Loadout")
    bool SelectPrimaryById(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Operator Loadout")
    bool SelectSecondaryById(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Operator Loadout")
    bool SelectEquipmentById(FName ItemId);

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    FName GetSelectedPrimaryId() const { return SelectedPrimaryId; }

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    FName GetSelectedSecondaryId() const { return SelectedSecondaryId; }

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    FName GetSelectedEquipmentId() const { return SelectedEquipmentId; }

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    TSubclassOf<ATU_WeaponBase> GetSelectedPrimaryClass() const;

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    TSubclassOf<ATU_WeaponBase> GetSelectedSecondaryClass() const;

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    float GetSelectedPrimaryWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    float GetSelectedSecondaryWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    float GetSelectedEquipmentWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    float GetSelectedNonMeleeWeightKg() const;

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    TArray<FTUOperatorWeaponEntry> GetPrimaryItems() const { return PrimaryItems; }

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    TArray<FTUOperatorWeaponEntry> GetSecondaryItems() const { return SecondaryItems; }

    UFUNCTION(BlueprintPure, Category="Operator Loadout")
    TArray<FTUOperatorEquipmentEntry> GetEquipmentItems() const { return EquipmentItems; }

    UFUNCTION(BlueprintCallable, Category="Operator Loadout")
    void SetPrimaryItems(const TArray<FTUOperatorWeaponEntry>& NewItems, FName PreferredItemId = NAME_None);

    UFUNCTION(BlueprintCallable, Category="Operator Loadout")
    void SetSecondaryItems(const TArray<FTUOperatorWeaponEntry>& NewItems, FName PreferredItemId = NAME_None);

    UFUNCTION(BlueprintCallable, Category="Operator Loadout")
    void SetEquipmentItems(const TArray<FTUOperatorEquipmentEntry>& NewItems, FName PreferredItemId = NAME_None);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout|Primary")
    TArray<FTUOperatorWeaponEntry> PrimaryItems;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout|Secondary")
    TArray<FTUOperatorWeaponEntry> SecondaryItems;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout|Equipment")
    TArray<FTUOperatorEquipmentEntry> EquipmentItems;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout|Primary")
    FName SelectedPrimaryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout|Secondary")
    FName SelectedSecondaryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Operator Loadout|Equipment")
    FName SelectedEquipmentId = NAME_None;

private:
    int32 FindWeaponIndex(const TArray<FTUOperatorWeaponEntry>& Items, FName ItemId) const;
    int32 FindEquipmentIndex(FName ItemId) const;
    void EnsureValidSelections();
};
