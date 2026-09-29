#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TUExecutionTypes.h"
#include "TU_GameState.generated.h"
USTRUCT(BlueprintType)
struct THEUNIT_API FTURaidParticipantState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(BlueprintReadOnly) ETURaidPlayerOutcome Outcome = ETURaidPlayerOutcome::Active;
    UPROPERTY(BlueprintReadOnly) FName ExtractId;
    UPROPERTY(BlueprintReadOnly) float ExtractionEndTime = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bExtracting = false;
    UPROPERTY(BlueprintReadOnly) bool bOutcomePersisted = false;
    UPROPERTY(BlueprintReadOnly) bool bSavePending = false;
    UPROPERTY(BlueprintReadOnly) TArray<FTUTaskProgress> Tasks;
};
UCLASS()
class THEUNIT_API ATU_GameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated, BlueprintReadOnly) FGuid RaidId;
    UPROPERTY(Replicated, BlueprintReadOnly) float RaidEndTime = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float RaidElapsedTime = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bTraining = false;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FTURaidParticipantState> Participants;
};
