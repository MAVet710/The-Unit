#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TUFPVOSDWidget.generated.h"

class ATU_FPVDronePlayer;
class SBorder;
class STextBlock;

/** Lightweight native FPV OSD driven exclusively by the drone telemetry snapshot. */
UCLASS()
class THEUNIT_API UTUFPVOSDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetObservedDrone(ATU_FPVDronePlayer* Drone);
    void Refresh();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    TWeakObjectPtr<ATU_FPVDronePlayer> ObservedDrone;
    TSharedPtr<SBorder> VideoLossOverlay;
    TSharedPtr<STextBlock> WarningText;
    TSharedPtr<STextBlock> FlightText;
    TSharedPtr<STextBlock> BatteryText;
    TSharedPtr<STextBlock> LinkText;
    TSharedPtr<STextBlock> MotionText;
    TSharedPtr<STextBlock> MotorText;
    TSharedPtr<STextBlock> InputText;
};
