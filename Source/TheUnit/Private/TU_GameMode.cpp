#include "TU_GameMode.h"

#include "TU_PlayerController.h"

ATU_GameMode::ATU_GameMode()
{
    PlayerControllerClass = ATU_PlayerController::StaticClass();
}
