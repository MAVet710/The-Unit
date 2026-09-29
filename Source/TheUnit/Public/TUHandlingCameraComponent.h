#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "TUHandlingCameraComponent.generated.h"
/** Close body and stock surfaces need a nearby camera clip plane, not a scaled weapon. */
UCLASS()
class THEUNIT_API UTUHandlingCameraComponent : public UCameraComponent
{
    GENERATED_BODY()
public:
    virtual void GetCameraView(float DeltaTime,FMinimalViewInfo& DesiredView) override;
};
