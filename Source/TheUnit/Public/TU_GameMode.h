#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TU_GameState.h"
#include "TU_GameMode.generated.h"
class ATU_ExtractionZone;
class UTUHideoutLifecycleSubsystem;
UCLASS()
class THEUNIT_API ATU_GameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATU_GameMode();
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Logout(AController* Exiting) override;
    void StartRaid(FGuid InRaidId, float DurationSeconds, bool bInTraining);
    FGuid RegisterParticipant(APawn* Pawn);
    bool ResolvePlayerOutcome(APawn* Pawn, ETURaidPlayerOutcome Outcome, FName ExtractId = NAME_None);
    void RecordTaskEvent(APawn* Pawn, ETUTaskCondition Condition, FName TargetId, int32 Amount = 1, FGuid EventId = FGuid());
    bool RecordUniqueTaskEvent(APawn* Pawn, const FGuid& EventId, ETUTaskCondition Condition, FName TargetId, int32 Amount = 1);
    bool AddTask(APawn* Pawn, const FTUTaskDefinition& Definition);
    bool HandoverItem(APawn* Pawn, FName TaskId, const FGuid& ItemId);
    bool RecoverItem(APawn* Pawn, FName DefinitionId, const FGuid& ItemId);
    bool BeginExtraction(APawn* Pawn, ATU_ExtractionZone* Zone);
    void CancelExtraction(APawn* Pawn);
    void AdvanceRaidTime(float DeltaSeconds);
    const FTURaidParticipantState* FindParticipant(APawn* Pawn) const;
    const FTURaidParticipantState* FindParticipantById(const FGuid& PlayerId) const;
    bool SetParticipantLedger(APawn* Pawn, const FTUItemLedger& Ledger);
    const FTUItemLedger* GetParticipantLedger(APawn* Pawn) const;
    float GetRaidElapsedTime() const { return ElapsedTime; }
    bool IsTrainingRaid() const { return bTrainingRaid; }
#if WITH_DEV_AUTOMATION_TESTS
    void ConfigureTestLifecycle(UTUHideoutLifecycleSubsystem* Lifecycle) { LifecycleOverride = Lifecycle; }
#endif
    UPROPERTY(EditDefaultsOnly, Category="Raid") float RaidDurationSeconds = 1800.f;
    UPROPERTY(EditDefaultsOnly, Category="Raid") bool bTrainingRaid = false;
    UPROPERTY(EditDefaultsOnly, Category="Raid") TArray<FTUTaskDefinition> DefaultTasks;
private:
    UTUHideoutLifecycleSubsystem* GetLifecycle() const;
    FTURaidParticipantState* MutableParticipant(APawn* Pawn);
    void SyncPlayerState(APawn* Pawn);
    void ScheduleReturnIfResolved();
    TArray<FGuid> GetTaskRecipients(APawn* Pawn) const;
    bool ApplyTaskEvent(const FGuid& PlayerId,const FGuid& EventId,ETUTaskCondition Condition,FName TargetId,int32 Amount,const TArray<FGuid>& Recipients);
    bool FlushPendingTaskEvents(const FGuid& RequiredPlayerId);
    UPROPERTY(Transient) TObjectPtr<ATU_GameState> RaidState;
    UPROPERTY(Transient) TObjectPtr<UTUHideoutLifecycleSubsystem> LifecycleOverride;
    TMap<TWeakObjectPtr<APawn>, FGuid> PawnIds;
    TMap<FGuid, FTUItemLedger> Ledgers;
    TMap<FGuid, TWeakObjectPtr<ATU_ExtractionZone>> ExtractionZones;
    TMap<FGuid, TSet<FGuid>> SeenTaskEvents;
    TMap<FGuid, FTURaidOutcome> PendingOutcomes;
    TMap<FGuid, float> NextSaveRetryTime;
    TMap<FGuid, int32> SaveRetryCounts;
    struct FPendingTaskEvent
    {
        FGuid PlayerId;
        TArray<FGuid> Recipients;
        FGuid EventId;
        ETUTaskCondition Condition=ETUTaskCondition::Interact;
        FName TargetId;
        int32 Amount=1;
        float RetryAt=0.f;
    };
    TArray<FPendingTaskEvent> PendingTaskEvents;
    float ElapsedTime = 0.f;
    bool bRaidStarted = false;
    bool bReturnScheduled = false;
};
