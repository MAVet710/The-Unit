#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TUExecutionTypes.h"
#include "TU_InteractableBase.generated.h"
class USphereComponent;
class APawn;
class UStaticMeshComponent;
class UTextRenderComponent;
/** Authority interaction target. Clients submit intent through their owned character. */
UCLASS()
class THEUNIT_API ATU_InteractableBase : public AActor
{
    GENERATED_BODY()
public:
    ATU_InteractableBase();
    virtual void BeginPlay() override;
    void ConfigureObjective(FName InTargetId, ETUTaskCondition InCondition);
    virtual bool Interact(APawn* Pawn);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ObjectivePresentation) FName TargetId = TEXT("SecureIntel");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ObjectivePresentation) ETUTaskCondition Condition = ETUTaskCondition::Interact;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float InteractionRange = 250.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSingleUseGlobally = false;
    UPROPERTY(ReplicatedUsing=OnRep_ObjectivePresentation, BlueprintReadOnly) bool bConsumed = false;
protected:
    bool ValidatePawn(APawn* Pawn) const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> InteractionShape;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ObjectiveMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> ObjectiveLabel;
    UFUNCTION() void OnRep_ObjectivePresentation();
    FGuid EventId;
    TSet<FGuid> CreditedPlayers;
};
