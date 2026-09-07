#include "TU_GameMode.h"

#include "TU_ModularOperatorCharacter.h"

ATU_GameMode::ATU_GameMode()
{
    DefaultPawnClass = ATU_ModularOperatorCharacter::StaticClass();
}
