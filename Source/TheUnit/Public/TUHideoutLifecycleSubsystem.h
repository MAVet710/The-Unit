#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TUExecutionTypes.h"
#include "TUHideoutLifecycleSubsystem.generated.h"

class ATU_ArmedOperatorCharacter;
class UTUHideoutProgressionComponent;
class UTUHideoutSaveGame;
class UTUMissionPackageData;

/** Owns local hideout persistence, loadout persistence and HQ <-> mission travel state. */
UCLASS()
class THEUNIT_API UTUHideoutLifecycleSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    bool LoadProfile();
    bool RecoverUnresolvedDeployments();

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    bool SaveProfile();

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void ApplyHideoutState(UTUHideoutProgressionComponent* Progression) const;

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void CaptureHideoutState(const UTUHideoutProgressionComponent* Progression);

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void ApplyOperatorLoadout(ATU_ArmedOperatorCharacter* Operator) const;

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void CaptureOperatorLoadout(const ATU_ArmedOperatorCharacter* Operator);

    UFUNCTION(BlueprintCallable, Category="Hideout|Mission")
    bool DeployToMission(ATU_ArmedOperatorCharacter* Operator, const UTUMissionPackageData* MissionPackage);

    UFUNCTION(BlueprintCallable, Category="Hideout|Mission")
    bool ReturnToHideout(bool bOperationCompleted);

    UFUNCTION(BlueprintPure, Category="Hideout|Mission")
    bool IsMissionInProgress() const;

    UFUNCTION(BlueprintPure, Category="Hideout|Mission")
    FName GetActiveMissionId() const;

    UFUNCTION(BlueprintCallable, Category="Hideout|Mission")
    void SetHideoutMapName(FName MapName);

    UFUNCTION(BlueprintPure, Category="Hideout|Persistence")
    UTUHideoutSaveGame* GetProfile() const { return Profile; }

    bool BeginDeployment(const FGuid& RaidId, const FGuid& PlayerId, const FTUItemLedger& Ledger, bool bTraining);
    bool BeginDeploymentBatch(const TArray<FTUDeploymentRecord>& Deployments);
    bool CheckpointTasks(const FGuid& RaidId, const TArray<FTUTaskProgress>& Candidates);
    bool CommitRaidOutcome(const FTURaidOutcome& Outcome, const FTUItemLedger& Ledger, const TArray<FTUTaskProgress>& Tasks);
    bool CommitTaskHandover(const FGuid& PlayerId, FName TaskId, const FGuid& ItemId);
    bool RecordRaidAcquisition(const FGuid& RaidId, const FGuid& PlayerId, const FTUItemLedger& Acquired);
    FGuid GetActiveRaidIdForPlayer(const FGuid& PlayerId) const;
    FGuid GetLocalPlayerId() const;
    FGuid GetOrCreatePlayerId(const FString& StableKey);
    FGuid GetActiveRaidId() const;
    FTUItemLedger GetPlayerStash(const FGuid& PlayerId) const;
    bool CaptureInitialKit(const FTUItemLedger& Ledger);
    bool CaptureInitialKitForPlayer(const FGuid& PlayerId, const FTUItemLedger& Ledger);
    void ConfigureTestSlot(const FString& UniqueSlot);
    // Fail Nth write: 1=journal, 2=backup, 3=primary. Zero disables injection.
    void InjectSaveFailure(int32 RequestedWriteNumber) { FailWriteNumber = RequestedWriteNumber; }
    FString GetLastPersistenceError() const { return LastPersistenceError; }
    bool IsRecoveryBlocked() const { return bRecoveryBlocked; }

protected:
    bool PersistCandidate(UTUHideoutSaveGame* Candidate);
    bool WriteSlot(UTUHideoutSaveGame* Save, const FString& Slot);
    int32 FailWriteNumber = 0;
    int32 WriteNumber = 0;
    FString LastPersistenceError;
    bool bRecoveryBlocked = false;
    UPROPERTY(Transient)
    TObjectPtr<UTUHideoutSaveGame> Profile;

    UPROPERTY(EditDefaultsOnly, Category="Hideout|Persistence")
    FString SaveSlotName = TEXT("TheUnit_HideoutProfile");

    UPROPERTY(EditDefaultsOnly, Category="Hideout|Persistence")
    int32 SaveUserIndex = 0;
};
