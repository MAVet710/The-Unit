#include "TU_GameMode.h"

#include "TU_ModularOperatorCharacter.h"
#include "TU_PlayerController.h"

ATU_GameMode::ATU_GameMode()
{
    DefaultPawnClass = ATU_ModularOperatorCharacter::StaticClass();
    PlayerControllerClass = ATU_PlayerController::StaticClass();
}
