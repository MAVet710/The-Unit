#pragma once
#include "CoreMinimal.h"

struct FTUWeaponMountProfile
{
    FVector HipOffset = FVector(31.f, 2.5f, -10.f);
    FRotator HipRotation = FRotator(2.f, 0.f, -1.f);
    FVector ReloadOffset = FVector(29.f, 3.f, -11.f);
    FRotator ReloadRotation = FRotator(7.f, 4.f, -2.f);
    float ReloadClipBodyWeight = 0.05f;
};

/**
 * Original The Unit procedural presentation tuning. Gameplay recoil/spread remains
 * authoritative elsewhere; these values only describe what the operator sees.
 * Translation, weapon rotation and camera kick intentionally remain separate.
 */
struct FTUWeaponPresentationProfile
{
    float WeaponTranslationScale = 0.55f;
    float WeaponPitchScale = 0.80f;
    float WeaponYawScale = 0.55f;
    float WeaponRollScale = 0.14f;
    float AutoImpulseScale = 0.78f;
    float CameraPitchScale = 0.26f;
    float CameraYawScale = 0.16f;
    float CameraRollScale = 0.07f;
    float WeaponRecoveryRate = 15.f;
    float CameraRecoveryRate = 21.f;
    float SwayLocationCm = 0.16f;
    float SwayRotationDegrees = 0.20f;
    float SwayFrequencyHz = 1.15f;
};

/** Calibrated contacts and shoulder presentation, not generic weapon dimensions. */
namespace TUWeaponContact
{
    THEUNIT_API FTransform MagazineHandInActor();
    THEUNIT_API FTransform ControlHandInActor();
    THEUNIT_API FTransform FingerPose(const FName& Bone, const FTransform& AuthoredGrip,
        float MagazineWeight, float ControlWeight);
    THEUNIT_API FVector MagazineCenterInActor();
    THEUNIT_API FVector ActionControlInActor();
    THEUNIT_API FTUWeaponMountProfile ResolveMountProfile(FName WeaponId);
    THEUNIT_API FTUWeaponPresentationProfile ResolvePresentationProfile(FName WeaponId);
}
