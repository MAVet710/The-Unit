#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TU_GameMode.generated.h"

/**
 * Authoritative match rules for The Unit.
 * Coordinates the native tactical player-controller boundary while mission rules evolve.
 */
UCLASS()
class THEUNIT_API ATU_GameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATU_GameMode();
};
