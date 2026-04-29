#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_ObjectiveBase.generated.h"

/**
 * Base mission objective actor.
 * Will represent objective lifecycle states and replicate completion progress.
 */
UCLASS()
class THEUNIT_API ATU_ObjectiveBase : public AActor
{
    GENERATED_BODY()
};
