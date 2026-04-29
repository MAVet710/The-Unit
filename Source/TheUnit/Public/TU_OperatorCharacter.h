#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TU_OperatorCharacter.generated.h"

/**
 * Base controllable operator pawn.
 * Will own movement/combat integration points, inventory hooks, and interaction entry points.
 */
UCLASS()
class THEUNIT_API ATU_OperatorCharacter : public ACharacter
{
    GENERATED_BODY()
};
