#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TUWeaponAttachmentDefinition.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class ETUWeaponAttachmentSlot : uint8
{
    Optic UMETA(DisplayName="Optic"),
    Magnifier UMETA(DisplayName="Magnifier"),
    Laser UMETA(DisplayName="Laser / Illuminator"),
    Light UMETA(DisplayName="Weapon Light"),
    Foregrip UMETA(DisplayName="Foregrip"),
    Muzzle UMETA(DisplayName="Muzzle Device"),
    Stock UMETA(DisplayName="Stock"),
    Magazine UMETA(DisplayName="Magazine"),
    SideAccessory UMETA(DisplayName="Side Accessory")
};

/** Data-only definition for a removable visual/gameplay rifle attachment. */
UCLASS(BlueprintType)
class THEUNIT_API UTUWeaponAttachmentDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment")
    ETUWeaponAttachmentSlot Slot = ETUWeaponAttachmentSlot::SideAccessory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment")
    FName AttachSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment")
    FTransform RelativeTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0.1", ClampMax="3.0"))
    float RecoilMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0.1", ClampMax="3.0"))
    float SpreadMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta=(ClampMin="0.0"))
    float WeightKg = 0.0f;
};
