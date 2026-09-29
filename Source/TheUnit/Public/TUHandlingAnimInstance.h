#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TUHandlingAnimInstance.generated.h"
class UAnimSequence;
/** Native evaluated sequence pose with world-space weapon contacts copied on the game thread. */
UCLASS(Transient)
class THEUNIT_API UTUHandlingAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Idle;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Reload;
    float IdleTime = 0.f;
    float ActionTime = 0.f;
    float ActionWeight = 0.f;
    float MagazineGripAlpha = 0.f;
    FTransform MagazineObjectWorld=FTransform::Identity;
    float ControlGripAlpha = 0.f;
    bool bContactsValid = false;
    bool bOverrideSupportTrajectory = false;
    float ReloadClipBodyWeightScale = 0.05f;
    float CrouchDrop = 0.f;
    FQuat AimRotationComponent = FQuat::Identity;
    FQuat EyeRotationComponent = FQuat::Identity;
    FTransform EvaluatedLeftTargetComponent = FTransform::Identity;
    FTransform EvaluatedRightTargetComponent = FTransform::Identity;
    bool bEvaluatedTargetsValid = false;
    static FTransform MapSupportToCurrentGrip(const FTransform& AuthoredLeft, const FTransform& AuthoredRight, const FTransform& PhysicalRight);
    static FVector FixedSegmentTranslation(const FVector& Animated, const FVector& Reference);
    FTransform RightGripWorld = FTransform::Identity;
    FTransform LeftGripWorld = FTransform::Identity;
    FTransform RestRightHand = FTransform::Identity;
    FTransform RestLeftHand = FTransform::Identity;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
