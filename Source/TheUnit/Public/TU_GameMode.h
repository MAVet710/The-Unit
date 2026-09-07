#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TU_GameMode.generated.h"

/**
 * Authoritative match rules for The Unit.
 * Will coordinate mission start/end, objective completion, and extraction outcomes.
 */
UCLASS()
class THEUNIT_API ATU_GameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATU_GameMode();
};
