#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TU_PlayerController.generated.h"

/**
 * Player input and local control bridge.
 * Will own tactical input handling, ping/callout requests, and player-facing UI commands.
 */
UCLASS()
class THEUNIT_API ATU_PlayerController : public APlayerController
{
    GENERATED_BODY()
};
