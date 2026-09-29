#include "TU_WorldLighting.h"
#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

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
    // Cool, restrained daylight keeps concrete/stucco readable without a
    // cinematic golden-hour treatment.
    Sun->SetIntensity(8.0f);
    Sun->SetAtmosphereSunLight(true);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("DaylightAtmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("DaylightSky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetIntensity(0.85f);
    Sky->SetRealTimeCapture(true);
    Sky->SetLowerHemisphereColor(FLinearColor(.06f,.07f,.09f));

    DistanceFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("UrbanDistanceFog"));
    DistanceFog->SetupAttachment(RootComponent);
    DistanceFog->SetMobility(EComponentMobility::Movable);
    DistanceFog->SetFogDensity(0.0045f);
    DistanceFog->SetFogHeightFalloff(0.16f);
    DistanceFog->SetFogInscatteringColor(FLinearColor(0.56f, 0.61f, 0.64f));
    DistanceFog->SetVolumetricFog(true);
    DistanceFog->SetVolumetricFogScatteringDistribution(0.25f);
    DistanceFog->SetVolumetricFogExtinctionScale(0.55f);
    DistanceFog->SetVolumetricFogAlbedo(FColor(205, 215, 220));
    DistanceFog->SetVolumetricFogStartDistance(700.0f);
    DistanceFog->SetVolumetricFogDistance(32000.0f);

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
