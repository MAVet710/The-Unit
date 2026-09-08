#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TU_PlayerController.generated.h"

/** Player input and local-control bridge for tactical UI commands. */
UCLASS()
class THEUNIT_API ATU_PlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void SetupInputComponent() override;

private:
    void ToggleMX50();
};
