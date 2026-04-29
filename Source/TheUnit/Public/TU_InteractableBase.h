#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_InteractableBase.generated.h"

/**
 * Base world interaction actor.
 * Will support shared interaction prompts, authority checks, and interaction outcome events.
 */
UCLASS()
class THEUNIT_API ATU_InteractableBase : public AActor
{
    GENERATED_BODY()
};
