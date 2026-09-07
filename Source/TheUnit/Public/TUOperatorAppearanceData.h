#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TUOperatorAppearanceData.generated.h"

class UAnimInstance;
class USkeletalMesh;
class UTUOperatorLoadoutData;

/** Base body/arms and default equipment for one operator appearance. */
UCLASS(BlueprintType)
class THEUNIT_API UTUOperatorAppearanceData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    TSoftObjectPtr<USkeletalMesh> ThirdPersonBodyMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    TSubclassOf<UAnimInstance> ThirdPersonAnimClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    FTransform ThirdPersonMeshTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Third Person")
    bool bHideThirdPersonBodyFromOwner = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    TSoftObjectPtr<USkeletalMesh> FirstPersonArmsMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    TSubclassOf<UAnimInstance> FirstPersonAnimClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="First Person")
    FTransform FirstPersonArmsTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loadout")
    TObjectPtr<UTUOperatorLoadoutData> DefaultLoadout = nullptr;
};
