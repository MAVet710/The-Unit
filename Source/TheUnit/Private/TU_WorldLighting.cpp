#include "TU_WorldLighting.h"
#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"

ATU_WorldLighting::ATU_WorldLighting()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("LightingRoot"));
    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DaylightSun"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-42.f, -35.f, 0.f));
    // Non-photometric graybox calibration: pair this modest sun with the fixed
    // exposure below. It is intentionally not a 50,000-lux physical-camera rig.
    Sun->SetIntensity(10.f);
    Sun->SetAtmosphereSunLight(true);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("DaylightAtmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("DaylightSky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetIntensity(1.f);
    Sky->SetRealTimeCapture(true);
    Sky->SetLowerHemisphereColor(FLinearColor(.06f,.07f,.09f));

    BenchmarkExposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("BenchmarkExposure"));
    BenchmarkExposure->SetupAttachment(RootComponent);
    BenchmarkExposure->bUnbound = true;
    BenchmarkExposure->Priority = 100.f;
    BenchmarkExposure->BlendWeight = 1.f;
    FPostProcessSettings& Settings = BenchmarkExposure->Settings;
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Settings.AutoExposureApplyPhysicalCameraExposure = false;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = -1.f;
    Settings.bOverride_AutoExposureBiasCurve = true;
    Settings.AutoExposureBiasCurve = nullptr;
    // Keep diagnostic geometry silhouettes readable instead of bleeding bright pixels.
    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = 0.f;
    Settings.bOverride_MotionBlurAmount = true;
    Settings.MotionBlurAmount = 0.f;
}
