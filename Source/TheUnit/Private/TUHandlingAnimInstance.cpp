#include "TUHandlingAnimInstance.h"
#include "TUWeaponContactProfile.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimNodeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

struct FTUHandlingAnimProxy final : FAnimInstanceProxy
{
    explicit FTUHandlingAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    UAnimSequence* Idle = nullptr;
    UAnimSequence* Reload = nullptr;
    float IdleTime = 0.f, ActionTime = 0.f, Weight = 0.f;
    bool bContacts = false;
    float MagazineGripAlpha = 0.f, ControlGripAlpha = 0.f;
    bool bOverrideSupport = false;
    float ReloadClipBodyWeightScale = 0.05f;
    FTransform Right, Left, RestRight, RestLeft;
    FTransform MagazineInComponent=FTransform::Identity;
    float CrouchDrop = 0.f;
    FQuat AimRotation = FQuat::Identity;
    FQuat EyeRotation = FQuat::Identity;
    FTransform EvaluatedLeft, EvaluatedRight;
    bool bEvaluated = false;
    virtual void PostEvaluate(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::PostEvaluate(Instance);
        auto* Handling = CastChecked<UTUHandlingAnimInstance>(Instance);
        Handling->EvaluatedLeftTargetComponent = EvaluatedLeft;
        Handling->EvaluatedRightTargetComponent = EvaluatedRight;
        Handling->bEvaluatedTargetsValid = bEvaluated;
    }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Handling = CastChecked<UTUHandlingAnimInstance>(Instance);
        Idle = Handling->Idle; Reload = Handling->Reload;
        IdleTime = Handling->IdleTime; ActionTime = Handling->ActionTime; Weight = Handling->ActionWeight;
        bContacts = Handling->bContactsValid;
        MagazineGripAlpha = Handling->MagazineGripAlpha;
        ControlGripAlpha = Handling->ControlGripAlpha;
        bOverrideSupport = Handling->bOverrideSupportTrajectory;
        ReloadClipBodyWeightScale = Handling->ReloadClipBodyWeightScale;
        CrouchDrop = Handling->CrouchDrop; AimRotation = Handling->AimRotationComponent;
        EyeRotation = Handling->EyeRotationComponent;
        const FTransform Component = Instance->GetSkelMeshComponent()->GetComponentTransform();
        MagazineInComponent=Handling->MagazineObjectWorld.GetRelativeTransform(Component);
        Right = Handling->RightGripWorld.GetRelativeTransform(Component);
        Left = Handling->LeftGripWorld.GetRelativeTransform(Component);
        RestRight = Handling->RestRightHand; RestLeft = Handling->RestLeftHand;
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        bEvaluated = false;
        Output.ResetToRefPose();
        if (!Idle) return true;
        FAnimationPoseData IdleData(Output);
        Idle->GetAnimationPose(IdleData, FAnimExtractContext(static_cast<double>(IdleTime), false));
        if (Reload && Weight > SMALL_NUMBER)
        {
            FPoseContext Action(Output);
            Action.ResetToRefPose();
            FAnimationPoseData ActionData(Action);
            Reload->GetAnimationPose(ActionData, FAnimExtractContext(static_cast<double>(ActionTime), false));
            for (FCompactPoseBoneIndex Index : Output.Pose.ForEachBoneIndex())
            {
                FTransform Blended;
                const float BodyWeight = bOverrideSupport ? Weight * FMath::Clamp(ReloadClipBodyWeightScale, 0.f, 0.25f) : Weight;
                Blended.Blend(Output.Pose[Index], Action.Pose[Index], BodyWeight);
                Output.Pose[Index] = Blended;
            }
        }
        if (!bContacts) return true;
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        auto Bone = [&Bones](const TCHAR* Name)
        {
            return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetReferenceSkeleton().FindBoneIndex(Name)));
        };
        // Grip fingers are authored from a stable contact pose, not the generic
        // reload's open-palm keys. Wrist IK alone cannot establish a grasp.
        FPoseContext Grip(Output);
        Grip.ResetToRefPose();
        FAnimationPoseData GripData(Grip);
        Idle->GetAnimationPose(GripData,FAnimExtractContext(0.0,false));
        for(const TCHAR* Side : {TEXT("r"),TEXT("l")})
            for(const TCHAR* Finger : {TEXT("thumb"),TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")})
                for(const TCHAR* Joint : {TEXT("metacarpal"),TEXT("01"),TEXT("02"),TEXT("03")}) {
                    const FName Name(*FString::Printf(TEXT("%s_%s_%s"),Finger,Joint,Side));
                    const auto Index=Bone(*Name.ToString());
                    if(Index.IsValid()) Output.Pose[Index]=TUWeaponContact::FingerPose(Name,Grip.Pose[Index],MagazineGripAlpha,ControlGripAlpha);
                }
        // The idle/reload clips contain slightly different segment translations.
        // Preserve their directions, but use the rig's fixed lengths before both
        // source-hand sampling and IK. No per-frame arm growth or IK stretching.
        for (const TCHAR* Name : {TEXT("lowerarm_r"), TEXT("hand_r"), TEXT("lowerarm_l"), TEXT("hand_l")})
        {
            const auto Index = Bone(Name);
            if (Index.IsValid())
                Output.Pose[Index].SetTranslation(UTUHandlingAnimInstance::FixedSegmentTranslation(
                    Output.Pose[Index].GetTranslation(), Bones.GetRefPoseTransform(Index).GetTranslation()));
        }
        FCSPose<FCompactPose> CSPose;
        CSPose.InitPose(Output.Pose);
        const auto RightHand = Bone(TEXT("hand_r")), LeftHand = Bone(TEXT("hand_l"));
        const FTransform AuthoredRight = RightHand.IsValid() ? CSPose.GetComponentSpaceTransform(RightHand) : RestRight;
        const FTransform AuthoredLeft = LeftHand.IsValid() ? CSPose.GetComponentSpaceTransform(LeftHand) : RestLeft;
        // Both samples must come from the same pre-IK source pose. Solving the right
        // chain first must not change the frame used to map the left trajectory.
        FTransform LeftTarget = Left;
        if (Weight > SMALL_NUMBER && !bOverrideSupport)
            LeftTarget.Blend(Left, UTUHandlingAnimInstance::MapSupportToCurrentGrip(AuthoredLeft, AuthoredRight, Right), Weight);
        auto SetBone = [&](FCompactPoseBoneIndex Index, const FTransform& Transform)
        {
            TArray<FBoneTransform, TInlineAllocator<1>> Change;
            Change.Emplace(Index, Transform);
            CSPose.LocalBlendCSBoneTransforms(Change, 1.f);
        };
        // Crouch changes the pelvis height, then bends the legs around unchanged
        // world-space feet. Character's mesh compensation alone leaves a standing
        // skeleton while the capsule and physical eye have already moved down.
        const auto Pelvis = Bone(TEXT("pelvis"));
        const auto FootR = Bone(TEXT("foot_r")), FootL = Bone(TEXT("foot_l"));
        if (CrouchDrop > SMALL_NUMBER && Pelvis.IsValid() && FootR.IsValid() && FootL.IsValid())
        {
            const FTransform Feet[] = {CSPose.GetComponentSpaceTransform(FootR), CSPose.GetComponentSpaceTransform(FootL)};
            FTransform P = CSPose.GetComponentSpaceTransform(Pelvis);
            P.AddToTranslation(FVector(0, 0, -CrouchDrop));
            SetBone(Pelvis, P);
            int32 Side = 0;
            for (bool bRight : {true, false})
            {
                const auto Thigh = Bone(bRight ? TEXT("thigh_r") : TEXT("thigh_l"));
                const auto Calf = Bone(bRight ? TEXT("calf_r") : TEXT("calf_l"));
                const auto Foot = bRight ? FootR : FootL;
                if (!Thigh.IsValid() || !Calf.IsValid()) { ++Side; continue; }
                FTransform U = CSPose.GetComponentSpaceTransform(Thigh), L = CSPose.GetComponentSpaceTransform(Calf), H = CSPose.GetComponentSpaceTransform(Foot);
                const FVector Hint = U.GetLocation() + FVector(0, 80, -20);
                AnimationCore::SolveTwoBoneIK(U, L, H, Hint, Feet[Side].GetLocation(), false, 1.0, 1.0);
                H.SetRotation(Feet[Side++].GetRotation());
                TArray<FBoneTransform, TInlineAllocator<3>> Changes;
                Changes.Emplace(Thigh, U); Changes.Emplace(Calf, L); Changes.Emplace(Foot, H);
                CSPose.LocalBlendCSBoneTransforms(Changes, 1.f);
            }
        }
        // Aim only the proximal upper body. A rotation at the lower spine retains
        // every local bone translation, and therefore cannot lengthen either arm.
        const auto Spine = Bone(TEXT("spine_01"));
        if (Spine.IsValid())
        {
            FTransform Chest = CSPose.GetComponentSpaceTransform(Spine);
            const FQuat InitialRotation = Chest.GetRotation();
            Chest.SetRotation((FQuat::Slerp(FQuat::Identity, AimRotation, .32f) * InitialRotation).GetNormalized());
            SetBone(Spine, Chest);
            // Move the shoulders toward an overextended grip by rotating the spine,
            // not by translating a shoulder or stretching an IK segment.
            for (int32 Iteration = 0; Iteration < 12; ++Iteration)
            {
                FVector RotationVector = FVector::ZeroVector;
                for (bool bRight : {true, false})
                {
                    const auto Upper = Bone(bRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
                    const auto Lower = Bone(bRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
                    const auto Hand = bRight ? RightHand : LeftHand;
                    if (!Upper.IsValid() || !Lower.IsValid() || !Hand.IsValid()) continue;
                    const FVector U = CSPose.GetComponentSpaceTransform(Upper).GetLocation();
                    const FVector L = CSPose.GetComponentSpaceTransform(Lower).GetLocation();
                    const FVector H = CSPose.GetComponentSpaceTransform(Hand).GetLocation();
                    const FVector ToTarget = (bRight ? Right : LeftTarget).GetLocation() - U;
                    const float Reach = FVector::Distance(U, L) + FVector::Distance(L, H);
                    const float Excess = FMath::Max(0.f, ToTarget.Size() - Reach * .96f);
                    const FVector Lever = U - Chest.GetLocation();
                    RotationVector += FVector::CrossProduct(Lever, ToTarget.GetSafeNormal()) * Excess / FMath::Max(Lever.SizeSquared(), 1.f);
                }
                const float Angle = FMath::Min(RotationVector.Size() * .65f, FMath::DegreesToRadians(4.f));
                if (Angle < .0001f) break;
                const FQuat Candidate = FQuat(RotationVector.GetSafeNormal(), Angle) * Chest.GetRotation();
                // Bound total torso inclination independently of the hand-error gate.
                const FQuat Delta = Candidate * InitialRotation.Inverse();
                FVector Axis; float TotalAngle;
                Delta.ToAxisAndAngle(Axis, TotalAngle);
                Chest.SetRotation((FQuat(Axis, FMath::Min(TotalAngle, FMath::DegreesToRadians(18.f))) * InitialRotation).GetNormalized());
                SetBone(Spine, Chest);
            }
        }
        EvaluatedRight = Right; EvaluatedLeft = LeftTarget; bEvaluated = true;
        auto Solve = [&](const TCHAR* UpperName, const TCHAR* LowerName, const TCHAR* HandName, FTransform Target)
        {
            const auto Upper = Bone(UpperName), Lower = Bone(LowerName), Hand = Bone(HandName);
            if (!Upper.IsValid() || !Lower.IsValid() || !Hand.IsValid()) return;
            FTransform U = CSPose.GetComponentSpaceTransform(Upper);
            FTransform L = CSPose.GetComponentSpaceTransform(Lower);
            FTransform H = CSPose.GetComponentSpaceTransform(Hand);
            const FVector AuthoredHint = L.GetLocation() + (L.GetLocation() - U.GetLocation()).GetSafeNormal() * 20.f;
            // Authored reload elbows were designed for a different weapon frame;
            // reusing that pole can swing the upper arm through the owner's eye.
            // Keep the same shoulder/hand targets and lengths, selecting the
            // downward/outward elbow solution smoothly only during manipulation.
            const float Side = Hand == RightHand ? 1.f : -1.f;
            const FVector ShoulderHint = U.GetLocation() + EyeRotation.RotateVector(FVector(-8.f, Side * 34.f, -32.f));
            const FVector ReloadHint = U.GetLocation() + EyeRotation.RotateVector(FVector(6.f, Side * 44.f, -50.f));
            FVector Hint = Hand == RightHand
                ? FMath::Lerp(ShoulderHint, ReloadHint, Weight)
                : FMath::Lerp(AuthoredHint, ReloadHint, Weight);
            AnimationCore::SolveTwoBoneIK(U, L, H, Hint, Target.GetLocation(), false, 1.0, 1.0);
            H.SetRotation(Target.GetRotation());
            TArray<FBoneTransform, TInlineAllocator<3>> Changes;
            Changes.Emplace(Upper, U); Changes.Emplace(Lower, L); Changes.Emplace(Hand, H);
            CSPose.LocalBlendCSBoneTransforms(Changes, 1.f);
        };
        Solve(TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"), Right);
        Solve(TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"), LeftTarget);
        if(MagazineGripAlpha>SMALL_NUMBER) {
            // Finger roots stay on the hand. Non-stretch chains curl around the
            // measured magazine, not through it; the thumb opposes the fingers.
            const TCHAR* Names[]={TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky"),TEXT("thumb")};
            const FVector Tips[]={FVector(19.1f,2.25f,-4.4f),FVector(18.4f,2.25f,-5.4f),
                FVector(17.6f,2.25f,-7.04f),FVector(16.8f,2.25f,-8.9f),FVector(11.f,-2.25f,-4.5f)};
            for(int32 I=0;I<5;++I) {
                const auto A=Bone(*FString::Printf(TEXT("%s_01_l"),Names[I]));
                const auto B=Bone(*FString::Printf(TEXT("%s_02_l"),Names[I]));
                const auto C=Bone(*FString::Printf(TEXT("%s_03_l"),Names[I]));
                if(!A.IsValid() || !B.IsValid() || !C.IsValid()) continue;
                FTransform U=CSPose.GetComponentSpaceTransform(A),L=CSPose.GetComponentSpaceTransform(B),H=CSPose.GetComponentSpaceTransform(C);
                const FVector Target=MagazineInComponent.TransformPosition(Tips[I]);
                const FVector Pole=MagazineInComponent.TransformPosition(I==4?FVector(8.f,-4.f,-7.f):FVector(24.f,-2.f,Tips[I].Z));
                AnimationCore::SolveTwoBoneIK(U,L,H,Pole,Target,false,1.,1.);
                const FVector Direction=MagazineInComponent.TransformVectorNoScale(I==4?FVector::ForwardVector:-FVector::ForwardVector);
                H.SetRotation((FQuat::FindBetweenNormals(H.GetUnitAxis(EAxis::X),Direction.GetSafeNormal())*H.GetRotation()).GetNormalized());
                TArray<FBoneTransform,TInlineAllocator<3>> Changes;
                Changes.Emplace(A,U);Changes.Emplace(B,L);Changes.Emplace(C,H);
                CSPose.LocalBlendCSBoneTransforms(Changes,FMath::Clamp(MagazineGripAlpha,0.f,1.f));
            }
        }
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CSPose), Output.Pose);
        return true;
    }
};
FAnimInstanceProxy* UTUHandlingAnimInstance::CreateAnimInstanceProxy() { return new FTUHandlingAnimProxy(this); }
void UTUHandlingAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }

FTransform UTUHandlingAnimInstance::MapSupportToCurrentGrip(const FTransform& AuthoredLeft, const FTransform& AuthoredRight, const FTransform& PhysicalRight)
{
    return AuthoredLeft.GetRelativeTransform(AuthoredRight) * PhysicalRight;
}

FVector UTUHandlingAnimInstance::FixedSegmentTranslation(const FVector& Animated, const FVector& Reference)
{
    return Animated.IsNearlyZero() ? Reference : Animated.GetSafeNormal() * Reference.Size();
}
