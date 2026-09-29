#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

/** Calibration describes geometry in the canonical physical weapon actor frame. */
struct THEUNIT_API FTUHandlingSightProfile
{
    FName CalibrationId = NAME_None;
    FTransform SightInActor = FTransform::Identity;
    FTransform EyeInActor = FTransform::Identity;
    FVector FrontSightInActor = FVector::ZeroVector;
    bool bHasFrontSight = false;
};

namespace TUHandlingSight
{
    /** False means unsupported geometry: callers retain the ordinary physical carry pose. */
    THEUNIT_API bool ResolveProfile(const UStaticMeshComponent* Mesh, FTUHandlingSightProfile& OutProfile);
    /** Maps the calibrated eye frame onto the physical eye; canonical forward is +X. */
    THEUNIT_API FTransform SolveWeaponWorld(const FTransform& EyeWorld, const FTUHandlingSightProfile& Profile);
}
