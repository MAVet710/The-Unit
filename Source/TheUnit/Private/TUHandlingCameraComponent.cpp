#include "TUHandlingCameraComponent.h"
#include "Camera/CameraTypes.h"
void UTUHandlingCameraComponent::GetCameraView(float DeltaTime,FMinimalViewInfo& DesiredView)
{
    Super::GetCameraView(DeltaTime,DesiredView);
    // Only the view's near plane changes. Actor transforms, visible geometry,
    // obstruction queries and the authoritative muzzle all remain world-space.
    DesiredView.PerspectiveNearClipPlane=2.f;
}
