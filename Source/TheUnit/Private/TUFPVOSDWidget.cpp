#include "TUFPVOSDWidget.h"

#include "TU_FPVDronePlayer.h"
#include "TUFPVSignalComponent.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SOverlay.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Widgets/Text/STextBlock.h"

void UTUFPVOSDWidget::SetObservedDrone(ATU_FPVDronePlayer* Drone)
{
    ObservedDrone = Drone;
    Refresh();
}

TSharedRef<SWidget> UTUFPVOSDWidget::RebuildWidget()
{
    const FSlateColor PrimaryColor(FLinearColor(0.80f, 1.0f, 0.80f, 0.95f));
    const FSlateColor WarningColor(FLinearColor(1.0f, 0.35f, 0.15f, 1.0f));

    return SNew(SOverlay)
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SAssignNew(VideoLossOverlay, SBorder)
            .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
        ]
        + SOverlay::Slot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Padding(FMargin(28.0f, 24.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(WarningText, STextBlock)
                .ColorAndOpacity(WarningColor)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(FlightText, STextBlock)
                .ColorAndOpacity(PrimaryColor)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(BatteryText, STextBlock)
                .ColorAndOpacity(PrimaryColor)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(LinkText, STextBlock)
                .ColorAndOpacity(PrimaryColor)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(MotionText, STextBlock)
                .ColorAndOpacity(PrimaryColor)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(MotorText, STextBlock)
                .ColorAndOpacity(PrimaryColor)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(InputText, STextBlock)
                .ColorAndOpacity(PrimaryColor)
            ]
        ];
}

void UTUFPVOSDWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    VideoLossOverlay.Reset();
    WarningText.Reset();
    FlightText.Reset();
    BatteryText.Reset();
    LinkText.Reset();
    MotionText.Reset();
    MotorText.Reset();
    InputText.Reset();
}

void UTUFPVOSDWidget::Refresh()
{
    ATU_FPVDronePlayer* Drone = ObservedDrone.Get();
    if (!Drone)
    {
        return;
    }

    const FFPVTelemetry T = Drone->GetTelemetry();
    const bool bDigital = Drone->GetVideoLinkType() == ETUFPVVideoLinkType::Digital;

    if (FlightText.IsValid())
    {
        const TCHAR* Mode = T.FlightMode == ETUFPVFlightMode::Acro ? TEXT("ACRO") : TEXT("STAB");
        FlightText->SetText(FText::FromString(FString::Printf(TEXT("%s  |  %s"), T.bArmed ? TEXT("ARMED") : TEXT("DISARMED"), Mode)));
    }

    if (BatteryText.IsValid())
    {
        BatteryText->SetText(FText::FromString(FString::Printf(TEXT("BAT  %.2f V   %3.0f%%   %.1f A"), T.BatteryVoltage, T.BatteryPercent, T.CurrentDrawAmps)));
    }

    if (LinkText.IsValid())
    {
        LinkText->SetText(FText::FromString(FString::Printf(
            TEXT("LINK  V:%3.0f%%  C:%3.0f%%  %.0f ms  %.0f m"),
            T.VideoQuality * 100.0f,
            T.ControlQuality * 100.0f,
            T.VideoLatencyMs,
            T.LinkDistanceMeters)));
    }

    if (MotionText.IsValid())
    {
        MotionText->SetText(FText::FromString(FString::Printf(TEXT("SPD  %.1f m/s   ALT  %.1f m   WASH %.0f%%"), T.SpeedMetersPerSecond, T.AltitudeAGLMeters, T.PropwashIntensity * 100.0f)));
    }

    if (MotorText.IsValid())
    {
        MotorText->SetText(FText::FromString(FString::Printf(
            TEXT("PROP  FL:%3.0f  FR:%3.0f  RR:%3.0f  RL:%3.0f"),
            T.MotorHealth.X * 100.0f,
            T.MotorHealth.Y * 100.0f,
            T.MotorHealth.Z * 100.0f,
            T.MotorHealth.W * 100.0f)));
    }

    if (InputText.IsValid())
    {
        InputText->SetText(FText::FromString(FString::Printf(TEXT("INPUT  %s   VIDEO %s"), Drone->IsRawRadioEnabled() ? TEXT("USB RC") : TEXT("GAMEPAD/KB"), bDigital ? TEXT("DIGITAL") : TEXT("ANALOG"))));
    }

    FString Warning;
    if (T.bSignalFailsafe)
    {
        Warning += TEXT("CONTROL LINK LOST / FAILSAFE");
    }
    if (T.bLowBattery)
    {
        if (!Warning.IsEmpty()) Warning += TEXT("   |   ");
        Warning += TEXT("LOW BATTERY");
    }
    const float MinimumMotorHealth = FMath::Min(FMath::Min(T.MotorHealth.X, T.MotorHealth.Y), FMath::Min(T.MotorHealth.Z, T.MotorHealth.W));
    if (MinimumMotorHealth < 0.80f)
    {
        if (!Warning.IsEmpty()) Warning += TEXT("   |   ");
        Warning += TEXT("PROP DAMAGE");
    }
    if (T.VideoQuality < 0.35f)
    {
        if (!Warning.IsEmpty()) Warning += TEXT("   |   ");
        Warning += T.VideoQuality < 0.10f ? TEXT("VIDEO LINK LOST") : TEXT("VIDEO DEGRADED");
    }
    if (Drone->WasArmBlockedByThrottle())
    {
        if (!Warning.IsEmpty()) Warning += TEXT("   |   ");
        Warning += TEXT("ARM BLOCKED: LOWER THROTTLE");
    }

    if (WarningText.IsValid())
    {
        WarningText->SetText(FText::FromString(Warning));
    }

    if (VideoLossOverlay.IsValid())
    {
        float BlackoutAlpha = 0.0f;
        if (bDigital && T.VideoQuality < 0.30f)
        {
            const float Loss = 1.0f - (T.VideoQuality / 0.30f);
            const float Flicker = FMath::FRandRange(0.35f, 1.0f);
            BlackoutAlpha = FMath::Clamp(Loss * Flicker, 0.0f, 0.96f);
        }
        VideoLossOverlay->SetBorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, BlackoutAlpha));
    }
}
