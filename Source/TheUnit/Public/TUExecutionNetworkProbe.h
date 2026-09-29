#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUExecutionNetworkProbe.generated.h"

class ATU_ArmedOperatorCharacter;
class ATU_GameMode;

/** Inert unless -TUNetworkSmoke=<run-id>; Development two-process regression driver. */
UCLASS()
class THEUNIT_API UTUExecutionNetworkProbe : public UActorComponent
{
    GENERATED_BODY()
public:
    UTUExecutionNetworkProbe();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    bool ApplyProfile();
    void ServerTick();
    void LocalTick();
    FString PawnSnapshot() const;
    FString CaptureGeometry() const;
    void SendCheckpoint(int32 Checkpoint);
    void Fail(const FString& Reason);
    void WriteResult(bool bPassed, const FString& Reason, const FString& Role);
    void ScheduleProcessExit(float DelaySeconds);
    bool IsEnabledAuthority() const;
    UFUNCTION(Server, Reliable) void ServerHello();
    UFUNCTION(Server, Reliable) void ServerAcknowledge(int32 Checkpoint, uint32 ObservedFingerprint);
    UFUNCTION(Server, Reliable) void ServerFinishReceipt();
    UFUNCTION(Client, Reliable) void ClientAction(int32 ActionNumber);
    UFUNCTION(Client, Reliable) void ClientCheckpoint(int32 Checkpoint, const FString& Snapshot);
    UFUNCTION(Client, Reliable) void ClientObserveOutcomes(int32 Checkpoint, FGuid HostPlayer, FGuid GuestPlayer, bool bGuestDead);
    UFUNCTION(Client, Reliable) void ClientFinish(bool bPassed, const FString& Reason);

    UPROPERTY(Transient) TObjectPtr<ATU_ArmedOperatorCharacter> ObservedPawn;
    UPROPERTY(Transient) TObjectPtr<ATU_ArmedOperatorCharacter> HostPawn;
    UPROPERTY(Transient) TObjectPtr<ATU_GameMode> RaidMode;
    FString RunId;
    FString ProfileName;
    FString ProcessRole;
    FString ExpectedSnapshot;
    FString StartMap;
    mutable FString GeometrySnapshot;
    FGuid HostId;
    FGuid GuestId;
    FGuid ReloadRequest;
    int32 ReloadRevision = 0;
    int32 InitialRounds = 0;
    int32 ReloadStartRevision = 0;
    int32 ServerStage = 0;
    int32 ExpectedCheckpoint = -1;
    int32 LastAcknowledged = -1;
    int32 PendingCheckpoint = -1;
    int32 LocalAction = 0;
    int32 MatchedCheckpoints = 0;
    uint32 ExpectedFingerprint = 0;
    float Elapsed = 0.f;
    float StageStarted = 0.f;
    float LastDiagnosticTime = 0.f;
    bool bEnabled = false;
    bool bProfileApplied = false;
    bool bHelloSent = false;
    bool bHelloReceived = false;
    bool bObserveOutcomes = false;
    bool bExpectGuestDead = false;
    bool bReceipt = false;
    bool bFinalOutcomesObserved = false;
    bool bFinished = false;
    bool bPassedResult = false;
};
