#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TUOperatorLoadoutData.generated.h"

class UTUEquipmentDefinition;

/** Named collection of operator equipment definitions. */
UCLASS(BlueprintType)
class THEUNIT_API UTUOperatorLoadoutData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loadout")
    FText DisplayName;

    /** One item per tactical slot is recommended. Later duplicate slots replace earlier entries. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loadout")
    TArray<TObjectPtr<UTUEquipmentDefinition>> Items;
};
