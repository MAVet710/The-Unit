#include "TU_PlayerState.h"
#include "Net/UnrealNetwork.h"
void ATU_PlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);
    if(ATU_PlayerState* Other=Cast<ATU_PlayerState>(PlayerState))
    { Other->PersistentPlayerId=PersistentPlayerId; Other->TaskProgress=TaskProgress; }
}
void ATU_PlayerState::OverrideWith(APlayerState* PlayerState)
{
    Super::OverrideWith(PlayerState);
    if(const ATU_PlayerState* Other=Cast<ATU_PlayerState>(PlayerState))
    { PersistentPlayerId=Other->PersistentPlayerId; TaskProgress=Other->TaskProgress; }
}
void ATU_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATU_PlayerState, PersistentPlayerId);
    DOREPLIFETIME_CONDITION(ATU_PlayerState, TaskProgress, COND_OwnerOnly);
}
