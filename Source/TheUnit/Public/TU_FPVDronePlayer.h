#pragma once

#include "CoreMinimal.h"
#include "TU_FPVDrone.h"
#include "TU_FPVDronePlayer.generated.h"

class UTUFPVOSDWidget;
class UTUFPVRadioInputComponent;
enum class ETUFPVVideoLinkType : uint8;

/** Player-facing FPV pawn layer: OSD, video-link presentation and USB RC input. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_FPVDronePlayer : public ATU_FPVDrone
{
    GENERATED_BODY()

public:
    ATU_FPVDronePlayer();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;

    UFUNCTION(BlueprintCallable, Category="FPV|Radio")
    void ToggleRawRadio();

    UFUNCTION(BlueprintPure, Category="FPV|Radio")
    bool IsRawRadioEnabled() const;

    UFUNCTION(BlueprintPure, Category="FPV|Radio")
    bool WasArmBlockedByThrottle() const { return bArmBlockedByThrottle; }

    UFUNCTION(BlueprintPure, Category="FPV|Video")
    ETUFPVVideoLinkType GetVideoLinkType() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FPV|Components")
    TObjectPtr<UTUFPVRadioInputComponent> RadioInput;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FPV|UI")
    TSubclassOf<UTUFPVOSDWidget> OSDWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Radio", meta=(ClampMin="0.0", ClampMax="0.25"))
    float ArmThrottleSafetyLimit = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Video", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumFilmGrain = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Video", meta=(ClampMin="0.0", ClampMax="5.0"))
    float MaximumChromaticAberration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Video", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumVideoJitterDegrees = 1.2f;

private:
    void UpdateRawRadio();
    void UpdateVideoPresentation(float DeltaSeconds);
    void CreateOSD();
    void DestroyOSD();

    UPROPERTY(Transient)
    TObjectPtr<UTUFPVOSDWidget> OSDWidget = nullptr;

    bool bArmBlockedByThrottle = false;
    bool bWasArmedLastFrame = false;
    float OSDRefreshAccumulator = 0.0f;
};
