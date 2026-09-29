#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TUHandlingCapture.generated.h"

class ATU_ArmedOperatorCharacter;
/** Development-only, opt-in rendered handling evidence. Never supplies inventory state. */
UCLASS()
class THEUNIT_API ATUHandlingCapture : public AActor
{
    GENERATED_BODY()
public:
    ATUHandlingCapture();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    static bool IsSafeCaptureId(const FString& Value);
    static bool IsSupportedRate(int32 Rate);
private:
    void OnPixels(int32 Width, int32 Height, const TArray<FColor>& Pixels);
    void Finish(bool bSucceeded, const TCHAR* Reason);
    void SetStage(int32 NewStage);
    bool VerifyPartsPickup();
    bool bPartsExtended = false, bPartsShotPassed = false, bPartsDropRequested = false;
    FGuid PartsDropId;
    bool bContactReview=false,bContactPickupPassed=false;
    UPROPERTY() TObjectPtr<AActor> ContactWall;
    int32 PartsInitialRounds = 0;
    UPROPERTY() TObjectPtr<ATU_ArmedOperatorCharacter> Operator;
    FString CaptureId, Directory, PendingMetrics, PendingImage;
    FString CapturedMetrics;
    FDelegateHandle ScreenshotHandle;
    double StartedAt = 0.;
    float Warmup = 0.f, StageTime = 0.f, PreviousAnimationTime = -1.f;
    int32 Rate = 60, Stage = -1, Frame = 0, Images = 0, ReloadSamples = 0, EvolvingSamples = 0;
    TSet<int32> ReloadPhases;
    FVector PreviousRight = FVector::ZeroVector, PreviousLeft = FVector::ZeroVector;
    bool bPending = false, bFinished = false, bInvalidMetrics = false, bReloadRequested = false;
};
