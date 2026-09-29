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
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> Atmosphere;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> DistanceFog;
    /** Global production post-process tuned for readable indoor/outdoor tactical contrast. */
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> BenchmarkExposure;
};
