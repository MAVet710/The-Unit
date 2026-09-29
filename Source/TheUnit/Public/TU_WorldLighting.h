#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_WorldLighting.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UPostProcessComponent;
class UExponentialHeightFogComponent;

/** Shared daylight for the generated benchmark maps, including remote peers. */
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
    /** One deterministic exposure for the graybox benchmark, independent of project EV range. */
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> BenchmarkExposure;
};
