#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_ExtractionZone.generated.h"
class APawn;
class UBoxComponent;
class UPrimitiveComponent;
class ATU_GameMode;
UCLASS(Blueprintable)
class THEUNIT_API ATU_ExtractionZone : public AActor
{
    GENERATED_BODY()
public:
    ATU_ExtractionZone();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintCallable, Category="Extraction") bool ExtractNow(bool bOperationCompleted = true);
    UFUNCTION(BlueprintPure, Category="Extraction") bool IsExtractionPending() const;
    bool IsEligible(APawn* Pawn, const ATU_GameMode* Mode) const;
    bool ContainsPawn(const APawn* Pawn) const;
    void CommitCapacity();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") FName ExtractId = TEXT("MainExit");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") float HoldSeconds = 3.f;
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category="Extraction") bool bPowered = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") bool bRequiresPower = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") FName RequiredItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") float OpensAtSeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") float ClosesAtSeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") int32 Capacity = 0;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Extraction") int32 UsedCapacity = 0;
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Trigger;
private:
    UFUNCTION() void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION() void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 BodyIndex);
};
