#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TU_PlayerState.generated.h"

/**
 * Replicated per-player state.
 * Will track operator stats, downed/extracted states, and mission rewards.
 */
UCLASS()
class THEUNIT_API ATU_PlayerState : public APlayerState
{
    GENERATED_BODY()
};
