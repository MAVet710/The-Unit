#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TU_GameInstance.generated.h"

/**
 * Core game instance for The Unit.
 * Will eventually own global session state, frontend flow, and persistent progression hooks.
 */
UCLASS()
class THEUNIT_API UTU_GameInstance : public UGameInstance
{
    GENERATED_BODY()
};
