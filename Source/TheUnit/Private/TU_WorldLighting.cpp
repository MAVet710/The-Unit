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
    // Midday overcast-bright exterior. Physical-scale values are applied through
    // editable neutral-daylight controls so validation can tune light without a film grade.
    Sun->SetAtmosphereSunLight(true);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("DaylightAtmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("DaylightSky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetRealTimeCapture(true);
    Sky->SetLowerHemisphereColor(FLinearColor(.06f,.07f,.09f));

    DistanceFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("UrbanDistanceFog"));
    DistanceFog->SetupAttachment(RootComponent);
    DistanceFog->SetMobility(EComponentMobility::Movable);
    DistanceFog->SetFogInscatteringColor(FLinearColor(0.60f, 0.64f, 0.67f));
    DistanceFog->SetVolumetricFog(true);
    DistanceFog->SetVolumetricFogScatteringDistribution(0.25f);
    DistanceFog->SetVolumetricFogExtinctionScale(0.38f);
    DistanceFog->SetVolumetricFogAlbedo(FColor(205, 215, 220));
    DistanceFog->SetVolumetricFogStartDistance(1000.0f);
    DistanceFog->SetVolumetricFogDistance(42000.0f);

    BenchmarkExposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("BenchmarkExposure"));
    BenchmarkExposure->SetupAttachment(RootComponent);
    BenchmarkExposure->bUnbound = true;
    BenchmarkExposure->Priority = 100.f;
    BenchmarkExposure->BlendWeight = 1.f;
    FPostProcessSettings& Settings = BenchmarkExposure->Settings;
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
    Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Settings.AutoExposureApplyPhysicalCameraExposure = false;
    Settings.bOverride_AutoExposureBias = true;
    Settings.bOverride_AutoExposureBiasCurve = true;
    Settings.AutoExposureBiasCurve = nullptr;
    // Subtle bloom only. The shooter remains clear while bright sky/specular response
    // still feels optical instead of perfectly digital.
    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = 0.15f;
    Settings.bOverride_MotionBlurAmount = true;
    Settings.MotionBlurAmount = 0.f;

    ApplyNeutralDaylightSettings();
}

void ATU_WorldLighting::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyNeutralDaylightSettings();
}

void ATU_WorldLighting::ApplyNeutralDaylightSettings()
{
    if (Sun)
    {
        Sun->SetIntensity(SunIntensityLux);
        Sun->SetUseTemperature(true);
        Sun->SetTemperature(SunTemperatureK);
    }
    if (Sky)
    {
        Sky->SetIntensity(SkyIntensity);
    }
    if (DistanceFog)
    {
        DistanceFog->SetFogDensity(FogDensity);
        DistanceFog->SetFogHeightFalloff(FogHeightFalloff);
    }
    if (BenchmarkExposure)
    {
        BenchmarkExposure->Settings.AutoExposureBias = ExposureBias;
    }
}
