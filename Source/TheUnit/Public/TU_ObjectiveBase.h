#pragma once
#include "CoreMinimal.h"
#include "TU_InteractableBase.h"
#include "TU_ObjectiveBase.generated.h"
class UPrimitiveComponent;
/** A reusable visit/interact/recovery objective; task completion never controls raid travel. */
UCLASS()
class THEUNIT_API ATU_ObjectiveBase : public ATU_InteractableBase
{
    GENERATED_BODY()
public:
    ATU_ObjectiveBase();
    virtual void BeginPlay() override;
    void BindEliminationTarget(AActor* Target);
    bool RegisterEliminationContributor(APawn* Pawn);
private:
    UFUNCTION() void OnVisit(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,int32 BodyIndex,bool bFromSweep,const FHitResult& SweepResult);
    UFUNCTION() void OnTargetDeath(AActor* Target);
    TWeakObjectPtr<AActor> EliminationTarget;
    TWeakObjectPtr<APawn> EliminatingPawn;
};
