#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TU_GameState.generated.h"

/**
 * Replicated match state container for The Unit.
 * Will expose mission timer, objective progress, and extraction state to clients.
 */
UCLASS()
class THEUNIT_API ATU_GameState : public AGameStateBase
{
    GENERATED_BODY()
};
