#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "TU_ExtractionZone.generated.h"

class APawn;
class UBoxComponent;
class UPrimitiveComponent;

/** Mission extraction area that returns the local operator to the persistent hideout. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_ExtractionZone : public AActor
{
    GENERATED_BODY()

public:
    ATU_ExtractionZone();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Extraction")
    bool ExtractNow(bool bOperationCompleted = true);

    UFUNCTION(BlueprintPure, Category="Extraction")
    bool IsExtractionPending() const { return bExtractionPending; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Extraction", meta=(ClampMin="0.0", ClampMax="30.0"))
    float ExtractionHoldSeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Extraction")
    bool bCountsAsOperationComplete = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Extraction")
    bool bRequireActiveMission = true;

private:
    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    void FinishTimedExtraction();

    TWeakObjectPtr<APawn> PendingPawn;
    FTimerHandle ExtractionTimer;
    bool bExtractionPending = false;
};
