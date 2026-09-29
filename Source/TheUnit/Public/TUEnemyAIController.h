#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "TUEnemyAIController.generated.h"
UENUM(BlueprintType) enum class ETUAIState:uint8 { Idle,Patrol,Suspicious,Investigate,Engage,Search,Dead };
UENUM(BlueprintType) enum class ETUAIArchetype:uint8 { Rifleman,Scout,Breacher,Support,Marksman,Leader };
UCLASS()
class THEUNIT_API ATUEnemyAIController:public AAIController
{
 GENERATED_BODY()
public:
 ATUEnemyAIController();
 virtual void Tick(float DeltaSeconds) override;
 void ConfigureArchetype(ETUAIArchetype V);
 float GetPreferredEngageRangeCm() const { return PreferredEngageRangeCm; }
 float GetSearchDurationSeconds() const { return SearchDurationSeconds; }
 void ReportStimulus(const FVector& Location,bool bVisual);
 void LoseContact();
 void MarkDead();
 ETUAIState GetTacticalState()const{return State;}
 ETUAIArchetype GetArchetype()const{return Archetype;}
protected:
 UPROPERTY(VisibleAnywhere) ETUAIState State=ETUAIState::Idle;
 UPROPERTY(EditAnywhere) ETUAIArchetype Archetype=ETUAIArchetype::Rifleman;
 FVector LastKnownLocation=FVector::ZeroVector;
 float StateSeconds=0.f;
 float PreferredEngageRangeCm=1800.f;
 float SearchDurationSeconds=8.f;
 FVector PatrolAnchor=FVector::ZeroVector;
 int32 PatrolStep=0;
 void IssuePatrolMove();
};
