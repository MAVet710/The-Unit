#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUFPVSignalComponent.generated.h"

UENUM(BlueprintType)
enum class ETUFPVVideoLinkType : uint8
{
    Analog UMETA(DisplayName="Analog"),
    Digital UMETA(DisplayName="Digital")
};

/** Distance/obstruction based control and video-link model. */
UCLASS(ClassGroup=(FPV), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUFPVSignalComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUFPVSignalComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="FPV|Signal")
    void SetSignalOrigin(AActor* NewOrigin);

    UFUNCTION(BlueprintPure, Category="FPV|Signal")
    float GetVideoQuality() const { return VideoQuality; }

    UFUNCTION(BlueprintPure, Category="FPV|Signal")
    float GetControlQuality() const { return ControlQuality; }

    UFUNCTION(BlueprintPure, Category="FPV|Signal")
    float GetDistanceMeters() const { return DistanceMeters; }

    UFUNCTION(BlueprintPure, Category="FPV|Signal")
    float GetVideoLatencyMs() const { return VideoLatencyMs; }

    UFUNCTION(BlueprintPure, Category="FPV|Signal")
    bool IsObstructed() const { return bObstructed; }

    UFUNCTION(BlueprintPure, Category="FPV|Signal")
    bool HasControlLink() const { return ControlQuality > ControlFailsafeThreshold; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Signal")
    ETUFPVVideoLinkType VideoLinkType = ETUFPVVideoLinkType::Digital;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Signal", meta=(ClampMin="1.0"))
    float VideoClearRangeMeters = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Signal", meta=(ClampMin="1.0"))
    float ControlClearRangeMeters = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Signal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ObstructionVideoMultiplier = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Signal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ObstructionControlMultiplier = 0.70f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Signal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ControlFailsafeThreshold = 0.04f;

private:
    float ComputeDistanceQuality(float Distance, float Range) const;
    void RefreshLink();

    UPROPERTY()
    TObjectPtr<AActor> SignalOrigin = nullptr;

    UPROPERTY(VisibleAnywhere, Category="FPV|Signal")
    float VideoQuality = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="FPV|Signal")
    float ControlQuality = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="FPV|Signal")
    float DistanceMeters = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="FPV|Signal")
    float VideoLatencyMs = 28.0f;

    UPROPERTY(VisibleAnywhere, Category="FPV|Signal")
    bool bObstructed = false;
};
