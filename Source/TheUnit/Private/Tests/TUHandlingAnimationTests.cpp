#include "Misc/AutomationTest.h"
#include "TUWeaponPresentationComponent.h"
#include "TwoBoneIK.h"
#include "TUHandlingAnimInstance.h"
#include "TU_OperatorCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHandlingContinuousClockTest, "TheUnit.Handling.ContinuousAuthorityClock", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHandlingContinuousClockTest::RunTest(const FString&)
{
    using P = ETUWeaponActionPhase;
    for (int32 Hz : {30, 60, 144})
    {
        float Previous = -1.f;
        for (int32 Frame = 0; Frame <= Hz; ++Frame)
        {
            const float Fraction = UTUWeaponPresentationComponent::PhaseAnimationFraction(P::Removed, 1.f - float(Frame)/Hz, 1.f);
            TestTrue(TEXT("Clock evolves continuously within committed phase"), Fraction > Previous);
            Previous = Fraction;
        }
    }
    TestEqual(TEXT("Phase boundary has identical pose time"), UTUWeaponPresentationComponent::PhaseAnimationFraction(P::Begin, 0.f, 1.f), UTUWeaponPresentationComponent::PhaseAnimationFraction(P::Removed, 1.f, 1.f));
    TestTrue(TEXT("Resume acquired cannot rewind removal"), UTUWeaponPresentationComponent::PhaseAnimationFraction(P::Acquired, 1.f, 1.f) >= .4f);
    TestEqual(TEXT("Late authority waits at phase endpoint"), UTUWeaponPresentationComponent::PhaseAnimationFraction(P::Inserted, -2.f, 1.f), .8f);
    FVector Joint, End;
    AnimationCore::SolveTwoBoneIK(FVector::ZeroVector, FVector(20,10,0), FVector(40,0,0), FVector(0,30,0), FVector(30,10,0), Joint, End, false, 1., 1.);
    TestTrue(TEXT("Reachable two-bone effector contacts target"), End.Equals(FVector(30,10,0), .001f));
    TestTrue(TEXT("IK preserves upper bone length"), FMath::IsNearlyEqual(Joint.Size(), FVector(20,10,0).Size(), .001));
    const FVector ReferenceSegment(27.f, 2.f, 0.f);
    for (double Scale : {.997, 1., 1.003})
    {
        const FVector SourceSegment = FVector(26.f, 7.f, 2.f) * Scale;
        const FVector Fixed = UTUHandlingAnimInstance::FixedSegmentTranslation(SourceSegment, ReferenceSegment);
        TestTrue(TEXT("Idle/reload source translation variation cannot change rig segment length"), FMath::IsNearlyEqual(Fixed.Size(), ReferenceSegment.Size(), .00001));
        TestTrue(TEXT("Length normalization retains authored segment direction"), Fixed.GetSafeNormal().Equals(SourceSegment.GetSafeNormal(), .00001));
    }
    // Support motion is defined by two simultaneously evaluated hands. A common
    // source-pose translation/rotation must cancel, including during reload.
    const FTransform AuthoredRight(FRotator(12, 37, -18), FVector(16, 42, 139));
    const FTransform AuthoredLeft(FRotator(-25, -11, 64), FVector(-12, 4, 103));
    const FTransform PhysicalRight(FRotator(5, -10, -12), FVector(40, 5, 130));
    const FTransform SourceMotion(FRotator(28, 19, 7), FVector(80, -40, 25));
    const FTransform Mapped = UTUHandlingAnimInstance::MapSupportToCurrentGrip(AuthoredLeft, AuthoredRight, PhysicalRight);
    const FTransform MovedSource = UTUHandlingAnimInstance::MapSupportToCurrentGrip(AuthoredLeft * SourceMotion, AuthoredRight * SourceMotion, PhysicalRight);
    TestTrue(TEXT("Common authored body motion cancels in current-right frame"), Mapped.Equals(MovedSource, .001f));
    TestTrue(TEXT("Current-right mapping preserves authored interhand distance"), FMath::IsNearlyEqual(FVector::Distance(Mapped.GetLocation(), PhysicalRight.GetLocation()), FVector::Distance(AuthoredLeft.GetLocation(), AuthoredRight.GetLocation()), .001));
    const FTransform PhysicalMotion(FRotator(-12, 60, 18), FVector(-20, 9, 40));
    TestTrue(TEXT("Left trajectory follows moving physical grip coherently"), UTUHandlingAnimInstance::MapSupportToCurrentGrip(AuthoredLeft, AuthoredRight, PhysicalRight * PhysicalMotion).Equals(Mapped * PhysicalMotion, .001f));
    // Forty-eight centimetres of actual crouch eye/pelvis drop bends the legs,
    // rather than moving the feet or increasing thigh/calf lengths.
    const auto* OperatorDefaults = GetDefault<ATU_OperatorCharacter>();
    const float StandingHalf = OperatorDefaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    const float CrouchedHalf = OperatorDefaults->GetCharacterMovement()->GetCrouchedHalfHeight();
    TestEqual(TEXT("Crouch physical eye drop matches pelvis compression"), 2.f * (StandingHalf - CrouchedHalf), 48.f);
    TestTrue(TEXT("Crouched physical eye stays within capsule"), CrouchedHalf - 14.f > 0.f && CrouchedHalf - 14.f < CrouchedHalf);
    const FVector Hip(0, 0, 52), Knee(0, 10, 7), Foot(0, 0, -48);
    const FVector OriginalFoot(0, 0, 0);
    FVector BentKnee, PlantedFoot;
    AnimationCore::SolveTwoBoneIK(Hip, Knee, Foot, FVector(0, 80, 30), OriginalFoot, BentKnee, PlantedFoot, false, 1., 1.);
    TestTrue(TEXT("Crouch feet stay planted"), PlantedFoot.Equals(OriginalFoot, .001));
    TestTrue(TEXT("Crouch thigh length remains fixed"), FMath::IsNearlyEqual(FVector::Distance(Hip, BentKnee), FVector::Distance(Hip, Knee), .001));
    TestTrue(TEXT("Crouch calf length remains fixed"), FMath::IsNearlyEqual(FVector::Distance(BentKnee, PlantedFoot), FVector::Distance(Knee, Foot), .001));
    return true;
}
#endif
