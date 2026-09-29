#include "TU_ObjectiveBase.h"
#include "TU_GameMode.h"
#include "TUHealthComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
ATU_ObjectiveBase::ATU_ObjectiveBase()
{
    InteractionShape->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
}
void ATU_ObjectiveBase::BeginPlay()
{
    Super::BeginPlay();
    InteractionShape->OnComponentBeginOverlap.AddDynamic(this,&ATU_ObjectiveBase::OnVisit);
}
void ATU_ObjectiveBase::OnVisit(UPrimitiveComponent*,AActor* Actor,UPrimitiveComponent*,int32,bool,const FHitResult&)
{
    if(Condition==ETUTaskCondition::Visit) Interact(Cast<APawn>(Actor));
}
void ATU_ObjectiveBase::BindEliminationTarget(AActor* Target)
{
    if(!HasAuthority() || !Target || Cast<APawn>(Target)==nullptr || Cast<APawn>(Target)->IsPlayerControlled()) return;
    if(UTUHealthComponent* H=Target->FindComponentByClass<UTUHealthComponent>())
    { EliminationTarget=Target; H->OnDeath.AddUniqueDynamic(this,&ATU_ObjectiveBase::OnTargetDeath); }
}
bool ATU_ObjectiveBase::RegisterEliminationContributor(APawn* Pawn)
{
    if(!HasAuthority() || !Pawn || !EliminationTarget.IsValid()) return false;
    ATU_GameMode* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>();
    if(!Mode || !Mode->FindParticipant(Pawn)) return false;
    EliminatingPawn=Pawn; return true;
}
void ATU_ObjectiveBase::OnTargetDeath(AActor* Target)
{
    if(!HasAuthority() || Target!=EliminationTarget.Get() || !EliminatingPawn.IsValid()) return;
    if(ATU_GameMode* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>())
    {
        if(!EventId.IsValid()) EventId=FGuid::NewGuid();
        Mode->RecordTaskEvent(EliminatingPawn.Get(),ETUTaskCondition::Eliminate,TargetId,1,EventId);
    }
}
