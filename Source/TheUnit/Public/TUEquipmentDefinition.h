#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TUEquipmentTypes.h"
#include "TUEquipmentDefinition.generated.h"

class USkeletalMesh;
class UStaticMesh;

/** Data-only definition for one removable operator equipment item. */
UCLASS(BlueprintType)
class THEUNIT_API UTUEquipmentDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
    ETUEquipmentSlot Slot = ETUEquipmentSlot::Accessory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ClampMin="0.0"))
    float WeightKg = 0.0f;

    /** Weighted gear using the operator skeleton. Preferred for vests/clothing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    TSoftObjectPtr<USkeletalMesh> ThirdPersonSkeletalMesh;

    /** Rigid gear attached to a socket. Preferred for helmets, radios, pouches, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    TSoftObjectPtr<UStaticMesh> ThirdPersonStaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    FName AttachSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    FTransform RelativeTransform = FTransform::Identity;

    /** If true and skeletons match, the skeletal gear follows the body pose directly. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    bool bUseLeaderPose = true;

    /** Prevents third-person gear from clipping the local first-person camera. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    bool bHideThirdPersonFromOwner = true;

    /** Optional dedicated first-person visual. Leave empty for gear that is not visible in first person. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    TSoftObjectPtr<USkeletalMesh> FirstPersonSkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    TSoftObjectPtr<UStaticMesh> FirstPersonStaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    FName FirstPersonAttachSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    FTransform FirstPersonRelativeTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    bool bUseFirstPersonLeaderPose = true;

    UFUNCTION(BlueprintPure, Category="Equipment")
    bool HasThirdPersonVisual() const
    {
        return !ThirdPersonSkeletalMesh.IsNull() || !ThirdPersonStaticMesh.IsNull();
    }

    UFUNCTION(BlueprintPure, Category="Equipment")
    bool HasFirstPersonVisual() const
    {
        return !FirstPersonSkeletalMesh.IsNull() || !FirstPersonStaticMesh.IsNull();
    }
};
