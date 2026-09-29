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
    // Midday overcast-bright exterior. Physical-scale sun intensity gives Lumen
    // enough energy for believable bounce while the post process preserves tactical contrast.
    Sun->SetIntensity(60000.0f);
    Sun->SetAtmosphereSunLight(true);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("DaylightAtmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("DaylightSky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetIntensity(1.05f);
    Sky->SetRealTimeCapture(true);
    Sky->SetLowerHemisphereColor(FLinearColor(.06f,.07f,.09f));

    DistanceFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("UrbanDistanceFog"));
    DistanceFog->SetupAttachment(RootComponent);
    DistanceFog->SetMobility(EComponentMobility::Movable);
    DistanceFog->SetFogDensity(0.0022f);
    DistanceFog->SetFogHeightFalloff(0.12f);
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
    Settings.AutoExposureBias = -0.35f;
    Settings.bOverride_AutoExposureBiasCurve = true;
    Settings.AutoExposureBiasCurve = nullptr;
    // Subtle bloom only. The shooter remains clear while bright sky/specular response
    // still feels optical instead of perfectly digital.
    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = 0.15f;
    Settings.bOverride_MotionBlurAmount = true;
    Settings.MotionBlurAmount = 0.f;
}
