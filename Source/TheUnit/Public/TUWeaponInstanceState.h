#pragma once

#include "CoreMinimal.h"
#include "TheUnitTypes.h"
#include "TUWeaponInstanceState.generated.h"

/**
 * Persistent per-item state for one weapon instance.
 * Immutable definitions remain in UTUWeaponDefinitionCatalog; inventory will own these instances.
 */
USTRUCT(BlueprintType)
struct THEUNIT_API FWeaponInstanceState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid InstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWeaponBuildState Build;

    /** Snapshot of mutable loaded state; magazine rounds exclude the chamber. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMagazineState MagazineState;

    /** Vertical-slice reserve snapshot; later inventory work may externalize reserve ammunition into item stacks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
    int32 AmmoReserve = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float ConditionNormalized = 1.0f;
};
