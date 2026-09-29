#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TUFrontEndGameMode.generated.h"
/** Menu has no operator, raid, deployed kit or reward-producing gameplay. */
UCLASS()
class THEUNIT_API ATUFrontEndGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATUFrontEndGameMode();
};
