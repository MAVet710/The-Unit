#include "TUEnemyAIController.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"

ATUEnemyAIController::ATUEnemyAIController(){PrimaryActorTick.bCanEverTick=true;}

void ATUEnemyAIController::ConfigureArchetype(ETUAIArchetype V)
{
    Archetype=V;
    switch(V)
    {
        case ETUAIArchetype::Scout: PreferredEngageRangeCm=2200.f; SearchDurationSeconds=12.f; break;
        case ETUAIArchetype::Breacher: PreferredEngageRangeCm=850.f; SearchDurationSeconds=6.f; break;
        case ETUAIArchetype::Support: PreferredEngageRangeCm=1700.f; SearchDurationSeconds=10.f; break;
        case ETUAIArchetype::Marksman: PreferredEngageRangeCm=3200.f; SearchDurationSeconds=14.f; break;
        case ETUAIArchetype::Leader: PreferredEngageRangeCm=1900.f; SearchDurationSeconds=13.f; break;
        default: PreferredEngageRangeCm=1800.f; SearchDurationSeconds=8.f; break;
    }
    if(APawn* P=GetPawn()) PatrolAnchor=P->GetActorLocation();
}

void ATUEnemyAIController::IssuePatrolMove()
{
    APawn* P=GetPawn(); UWorld* W=GetWorld(); if(!P||!W)return;
    if(PatrolAnchor.IsNearlyZero()) PatrolAnchor=P->GetActorLocation();
    const float Radius=Archetype==ETUAIArchetype::Scout?1100.f:Archetype==ETUAIArchetype::Marksman?500.f:750.f;
    const float Angle=FMath::DegreesToRadians(float((PatrolStep++*137)%360));
    const FVector Desired=PatrolAnchor+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)*Radius;
    FNavLocation Nav;
    if(UNavigationSystemV1* N=UNavigationSystemV1::GetCurrent(W))
    {
        if(N->ProjectPointToNavigation(Desired,Nav,FVector(500,500,300)))
        {
            const EPathFollowingRequestResult::Type Result=MoveToLocation(Nav.Location,100.f,true,true,false,true);
            UE_LOG(LogTemp,Display,TEXT("TUAI NavMove archetype=%d state=%d result=%d target=%s"),(int32)Archetype,(int32)State,(int32)Result,*Nav.Location.ToCompactString());
        }
        else UE_LOG(LogTemp,Warning,TEXT("TUAI NavProjectionFailed archetype=%d desired=%s"),(int32)Archetype,*Desired.ToCompactString());
    }
}

void ATUEnemyAIController::ReportStimulus(const FVector& L,bool Visual)
{
    if(State==ETUAIState::Dead)return;
    LastKnownLocation=L; State=Visual?ETUAIState::Engage:ETUAIState::Suspicious; StateSeconds=0.f;
    if(Visual)
    {
        if(APawn* P=GetPawn())
        {
            const float Distance=FVector::Dist(P->GetActorLocation(),L);
            if(Archetype==ETUAIArchetype::Breacher && Distance>PreferredEngageRangeCm) MoveToLocation(L,PreferredEngageRangeCm*.7f);
            else if(Archetype==ETUAIArchetype::Marksman && Distance<PreferredEngageRangeCm*.65f)
            {
                const FVector Away=(P->GetActorLocation()-L).GetSafeNormal();
                MoveToLocation(P->GetActorLocation()+Away*900.f,100.f);
            }
        }
    }
}
void ATUEnemyAIController::LoseContact(){if(State==ETUAIState::Engage){State=ETUAIState::Search;StateSeconds=0.f;MoveToLocation(LastKnownLocation,125.f);}}
void ATUEnemyAIController::MarkDead(){State=ETUAIState::Dead;StopMovement();}
void ATUEnemyAIController::Tick(float D)
{
    Super::Tick(D); if(State==ETUAIState::Dead)return; StateSeconds+=FMath::Max(0.f,D);
    if(State==ETUAIState::Idle&&StateSeconds>1.f){State=ETUAIState::Patrol;StateSeconds=0.f;IssuePatrolMove();}
    else if(State==ETUAIState::Patrol&&StateSeconds>5.f){StateSeconds=0.f;IssuePatrolMove();}
    else if(State==ETUAIState::Suspicious&&StateSeconds>.35f){State=ETUAIState::Investigate;StateSeconds=0.f;MoveToLocation(LastKnownLocation,100.f);}
    else if(State==ETUAIState::Investigate&&StateSeconds>5.f){State=ETUAIState::Search;StateSeconds=0.f;IssuePatrolMove();}
    else if(State==ETUAIState::Search&&StateSeconds>SearchDurationSeconds){State=ETUAIState::Patrol;StateSeconds=0.f;IssuePatrolMove();}
}
