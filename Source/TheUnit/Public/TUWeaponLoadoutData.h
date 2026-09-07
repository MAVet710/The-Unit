#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TUWeaponLoadoutData.generated.h"

class UTUWeaponAttachmentDefinition;

/** Named collection of weapon attachments. Duplicate slots resolve last-item-wins. */
UCLASS(BlueprintType)
class THEUNIT_API UTUWeaponLoadoutData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loadout")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loadout")
    TArray<TObjectPtr<UTUWeaponAttachmentDefinition>> Attachments;
};
