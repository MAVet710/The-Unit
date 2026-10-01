#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_WorldLighting.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UPostProcessComponent;
class UExponentialHeightFogComponent;

/** Production daylight/atmosphere shared by generated maps and replicated peers. */
UCLASS()
class THEUNIT_API ATU_WorldLighting : public AActor
{
    GENERATED_BODY()
public:
    ATU_WorldLighting();
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting|Neutral Daylight", meta=(ClampMin="0.0"))
    float SunIntensityLux = 60000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting|Neutral Daylight", meta=(ClampMin="1000.0", ClampMax="15000.0"))
    float SunTemperatureK = 6500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting|Neutral Daylight", meta=(ClampMin="0.0"))
    float SkyIntensity = 1.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting|Neutral Daylight", meta=(ClampMin="0.0", ClampMax="0.05"))
    float FogDensity = 0.0022f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting|Neutral Daylight", meta=(ClampMin="0.001", ClampMax="2.0"))
    float FogHeightFalloff = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting|Neutral Daylight", meta=(ClampMin="-5.0", ClampMax="5.0"))
    float ExposureBias = -0.35f;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> Atmosphere;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> DistanceFog;
    /** Global production post-process tuned for readable indoor/outdoor tactical contrast. */
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> BenchmarkExposure;

    void ApplyNeutralDaylightSettings();
};
