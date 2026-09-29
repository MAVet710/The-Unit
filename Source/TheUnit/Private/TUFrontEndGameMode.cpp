#include "TUFrontEndGameMode.h"
#include "TU_PlayerController.h"
#include "GameFramework/HUD.h"
ATUFrontEndGameMode::ATUFrontEndGameMode()
{
    PlayerControllerClass=ATU_PlayerController::StaticClass();
    DefaultPawnClass=nullptr; HUDClass=AHUD::StaticClass();
    bStartPlayersAsSpectators=true;
}
