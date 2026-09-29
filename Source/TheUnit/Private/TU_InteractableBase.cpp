#include "TU_InteractableBase.h"
#include "TU_GameMode.h"
#include "TUHealthComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
ATU_InteractableBase::ATU_InteractableBase()
{
    bReplicates=true;
    SetReplicateMovement(true);
    InteractionShape=CreateDefaultSubobject<USphereComponent>(TEXT("InteractionShape"));
    SetRootComponent(InteractionShape);
    InteractionShape->SetSphereRadius(45.f);
    InteractionShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionShape->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionShape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    ObjectiveMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectiveMesh"));
    ObjectiveMesh->SetupAttachment(InteractionShape);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Cube.Succeeded()) ObjectiveMesh->SetStaticMesh(Cube.Object);
    ObjectiveMesh->SetRelativeScale3D(FVector(.6f,.6f,.8f));
    ObjectiveMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ObjectiveLabel=CreateDefaultSubobject<UTextRenderComponent>(TEXT("ObjectiveLabel"));
    ObjectiveLabel->SetupAttachment(InteractionShape);
    ObjectiveLabel->SetRelativeLocation(FVector(0,0,80));
    ObjectiveLabel->SetWorldSize(24.f);
}
void ATU_InteractableBase::BeginPlay() { Super::BeginPlay(); OnRep_ObjectivePresentation(); }
void ATU_InteractableBase::ConfigureObjective(FName InTargetId,ETUTaskCondition InCondition)
{
    if(!HasAuthority()) return;
    TargetId=InTargetId; Condition=InCondition; OnRep_ObjectivePresentation(); ForceNetUpdate();
}
void ATU_InteractableBase::OnRep_ObjectivePresentation()
{
    ObjectiveLabel->SetText(FText::FromName(TargetId));
    ObjectiveMesh->SetVisibility(!bConsumed); ObjectiveLabel->SetVisibility(!bConsumed);
    InteractionShape->SetCollisionEnabled(bConsumed?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
}
void ATU_InteractableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ATU_InteractableBase,bConsumed);
    DOREPLIFETIME(ATU_InteractableBase,TargetId); DOREPLIFETIME(ATU_InteractableBase,Condition);
}
bool ATU_InteractableBase::ValidatePawn(APawn* Pawn) const
{
    if(!HasAuthority() || !Pawn || bConsumed || FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())>FMath::Square(InteractionRange)) return false;
    if(const UTUHealthComponent* H=Pawn->FindComponentByClass<UTUHealthComponent>()) if(H->IsDead()) return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(TaskInteraction),false,Pawn);
    if(GetWorld()->LineTraceSingleByChannel(Hit,Pawn->GetActorLocation(),GetActorLocation(),ECC_Visibility,Params) && Hit.GetActor()!=this) return false;
    return true;
}
bool ATU_InteractableBase::Interact(APawn* Pawn)
{
    if(!ValidatePawn(Pawn)) return false;
    ATU_GameMode* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>();
    const FTURaidParticipantState* P=Mode?Mode->FindParticipant(Pawn):nullptr;
    if(!P || CreditedPlayers.Contains(P->PlayerId)) return false;
    if(!EventId.IsValid()) EventId=FGuid::NewGuid();
    const bool Applied=Condition==ETUTaskCondition::Recover?Mode->RecoverItem(Pawn,TargetId,EventId):
        Mode->RecordUniqueTaskEvent(Pawn,EventId,Condition,TargetId);
    if(!Applied) return false;
    CreditedPlayers.Add(P->PlayerId);
    bConsumed=bSingleUseGlobally || Condition==ETUTaskCondition::Recover || Condition==ETUTaskCondition::Destroy;
    OnRep_ObjectivePresentation();
    ForceNetUpdate(); return true;
}
