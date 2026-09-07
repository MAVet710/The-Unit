#include "TU_FPVDronePlayer.h"

#include "TUFPVOSDWidget.h"
#include "TUFPVRadioInputComponent.h"
#include "TUFPVSignalComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"

ATU_FPVDronePlayer::ATU_FPVDronePlayer()
{
    RadioInput = CreateDefaultSubobject<UTUFPVRadioInputComponent>(TEXT("RadioInput"));
    OSDWidgetClass = UTUFPVOSDWidget::StaticClass();
}

void ATU_FPVDronePlayer::BeginPlay()
{
    Super::BeginPlay();

    if (FPVCamera)
    {
        FPVCamera->PostProcessBlendWeight = 1.0f;
        FPVCamera->PostProcessSettings.bOverride_FilmGrainIntensity = true;
        FPVCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
        FPVCamera->PostProcessSettings.bOverride_VignetteIntensity = true;
    }
}

void ATU_FPVDronePlayer::Tick(float DeltaSeconds)
{
    UpdateRawRadio();
    Super::Tick(DeltaSeconds);
    UpdateVideoPresentation(DeltaSeconds);

    OSDRefreshAccumulator += DeltaSeconds;
    if (OSDWidget && OSDRefreshAccumulator >= 0.05f)
    {
        OSDRefreshAccumulator = 0.0f;
        OSDWidget->Refresh();
    }

    bWasArmedLastFrame = bArmed;
}

void ATU_FPVDronePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAction(TEXT("FPVToggleRadio"), IE_Pressed, this, &ATU_FPVDronePlayer::ToggleRawRadio);
}

void ATU_FPVDronePlayer::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    CreateOSD();
}

void ATU_FPVDronePlayer::UnPossessed()
{
    DestroyOSD();
    Super::UnPossessed();
}

void ATU_FPVDronePlayer::ToggleRawRadio()
{
    if (!RadioInput)
    {
        return;
    }

    const bool bEnable = !RadioInput->IsEnabled();
    if (bEnable && bArmed)
    {
        ToggleArmed();
    }

    RadioInput->SetEnabled(bEnable);
    SetExternalRCEnabled(bEnable);
    bArmBlockedByThrottle = false;

    if (!bEnable)
    {
        SetExternalRCInput(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

bool ATU_FPVDronePlayer::IsRawRadioEnabled() const
{
    return RadioInput && RadioInput->IsEnabled();
}

ETUFPVVideoLinkType ATU_FPVDronePlayer::GetVideoLinkType() const
{
    return Signal ? Signal->VideoLinkType : ETUFPVVideoLinkType::Digital;
}

void ATU_FPVDronePlayer::UpdateRawRadio()
{
    if (!RadioInput || !RadioInput->IsEnabled())
    {
        SetExternalRCEnabled(false);
        return;
    }

    SetExternalRCEnabled(true);

    APlayerController* PlayerController = Cast<APlayerController>(Controller);
    const FFPVRadioChannels Channels = RadioInput->ReadChannels(PlayerController);
    if (!Channels.bValid)
    {
        SetExternalRCInput(0.0f, 0.0f, 0.0f, 0.0f);
        if (bArmed)
        {
            ToggleArmed();
        }
        return;
    }

    SetExternalRCInput(Channels.Throttle, Channels.Roll, Channels.Pitch, Channels.Yaw);

    if (bArmed && !bWasArmedLastFrame && Channels.Throttle > ArmThrottleSafetyLimit)
    {
        ToggleArmed();
        bArmBlockedByThrottle = true;
    }
    else if (!bArmed && Channels.Throttle <= ArmThrottleSafetyLimit)
    {
        bArmBlockedByThrottle = false;
    }
}

void ATU_FPVDronePlayer::UpdateVideoPresentation(float DeltaSeconds)
{
    if (!FPVCamera)
    {
        return;
    }

    const float Quality = Signal ? FMath::Clamp(Signal->GetVideoQuality(), 0.0f, 1.0f) : 1.0f;
    const float Loss = 1.0f - Quality;
    const bool bDigital = GetVideoLinkType() == ETUFPVVideoLinkType::Digital;
    const float Severity = bDigital
        ? FMath::Clamp((0.65f - Quality) / 0.65f, 0.0f, 1.0f)
        : Loss;

    FPVCamera->PostProcessSettings.FilmGrainIntensity = bDigital
        ? (0.015f + Severity * MaximumFilmGrain * 0.35f)
        : (0.03f + Severity * MaximumFilmGrain);
    FPVCamera->PostProcessSettings.SceneFringeIntensity = Severity * MaximumChromaticAberration;
    FPVCamera->PostProcessSettings.VignetteIntensity = 0.15f + (Severity * 0.35f);

    const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const float Jitter = Severity * MaximumVideoJitterDegrees;
    const float PitchNoise = FMath::Sin(Time * 31.0f) * Jitter * 0.35f;
    const float YawNoise = FMath::Sin((Time * 41.0f) + 1.7f) * Jitter * 0.45f;
    const float RollNoise = FMath::Sin((Time * 53.0f) + 0.6f) * Jitter;
    FPVCamera->SetRelativeRotation(FRotator(CameraTiltDeg + PitchNoise, YawNoise, RollNoise));

    (void)DeltaSeconds;
}

void ATU_FPVDronePlayer::CreateOSD()
{
    if (OSDWidget || !OSDWidgetClass)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(Controller);
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    OSDWidget = CreateWidget<UTUFPVOSDWidget>(PlayerController, OSDWidgetClass);
    if (OSDWidget)
    {
        OSDWidget->SetObservedDrone(this);
        OSDWidget->AddToPlayerScreen(100);
    }
}

void ATU_FPVDronePlayer::DestroyOSD()
{
    if (OSDWidget)
    {
        OSDWidget->RemoveFromParent();
        OSDWidget = nullptr;
    }
}
