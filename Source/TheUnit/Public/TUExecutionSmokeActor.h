#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TUExecutionSmokeActor.generated.h"

class ATU_ArmedOperatorCharacter;
class ATU_GameMode;
class UTUHideoutLifecycleSubsystem;
class UTUHideoutSaveGame;

/** Opt-in Development executable smoke driver; never spawned for ordinary play. */
UCLASS(NotBlueprintable)
class THEUNIT_API ATUExecutionSmokeActor : public AActor
{
    GENERATED_BODY()
public:
    ATUExecutionSmokeActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void Finish(bool bPassed, const FString& Reason);
    FString Snapshot(const UTUHideoutSaveGame* Profile) const;
    bool WarmupReady();
    void IsolatePhysicalInput();
    void LogWeaponState(const TCHAR* Label) const;
    void TickSpawnCheck();
    TWeakObjectPtr<ATU_ArmedOperatorCharacter> InputIsolatedPawn;
    int32 SpawnGroundedSamples = 0;
    float SpawnMinHeight = 0.f;
    float SpawnMaxHeight = 0.f;
    float SpawnCameraDistance = 0.f;
    float SpawnSupportDistance = 0.f;
    FString Mode;
    FString ProfileSlot;
    FString EvidenceBase;
    float Elapsed = 0.f;
    float LastShotTime = -1.f;
    float WarmupSeconds = 0.f;
    float PawnReadyTime = -1.f;
    int32 Stage = 0;
    int32 Shots = 0;
    int32 InitialRounds = 0;
    bool bFinished = false;
    bool bRoundTrip = false;
    bool bDeathScenario = false;
    bool bTakeScreenshot = false;
    bool bScreenshotRequested = false;
    UPROPERTY(Transient) TObjectPtr<ATU_ArmedOperatorCharacter> Operator;
    UPROPERTY(Transient) TObjectPtr<ATU_GameMode> RaidMode;
    UPROPERTY(Transient) TObjectPtr<UTUHideoutLifecycleSubsystem> Lifecycle;
};
