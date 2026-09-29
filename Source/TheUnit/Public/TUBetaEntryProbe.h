#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "TUBetaEntryProbe.generated.h"
class ATU_PlayerController;
class IInputProcessor;
/** Opt-in development-only end-to-end runner; never enabled by a normal launch. */
UCLASS()
class THEUNIT_API UTUBetaEntryProbe : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    bool TickProbe(float DeltaSeconds);
    void Next(int32 Stage);
    bool Require(bool Condition,const FString& Reason);
    void Capture(const FString& Name);
    void Finish(bool Passed,const FString& Reason);
    bool CheckPreferences(ATU_PlayerController* PC);
    FTSTicker::FDelegateHandle Handle;
    TSharedPtr<IInputProcessor> InputGuard;
    FString Id,Mode,Directory;
    FString PendingCapture;
    double PendingCaptureAt=0.;
    int32 Stage=0;
    double Started=0,StageStarted=0;
    float FrozenTime=0;
    FIntPoint PreviousResolution;
    bool bFinished=false;
    bool bReadyToRun=false;
    TArray<FString> Evidence;
};
