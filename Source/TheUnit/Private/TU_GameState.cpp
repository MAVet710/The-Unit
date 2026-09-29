#include "TU_GameState.h"
#include "Net/UnrealNetwork.h"
void ATU_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATU_GameState, RaidId);
    DOREPLIFETIME(ATU_GameState, RaidEndTime);
    DOREPLIFETIME(ATU_GameState, RaidElapsedTime);
    DOREPLIFETIME(ATU_GameState, bTraining);
    DOREPLIFETIME(ATU_GameState, Participants);
}
