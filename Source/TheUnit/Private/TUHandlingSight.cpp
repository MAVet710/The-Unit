#include "TUHandlingSight.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Math/RotationMatrix.h"

FTransform TUHandlingSight::SolveWeaponWorld(const FTransform& EyeWorld, const FTUHandlingSightProfile& Profile)
{
    // Unreal composes local * parent. This solves EyeInActor * WeaponWorld = EyeWorld.
    // Camera-only recoil must never be supplied as EyeWorld by the caller.
    return Profile.EyeInActor.Inverse() * EyeWorld;
}

bool TUHandlingSight::ResolveProfile(const UStaticMeshComponent* Mesh, FTUHandlingSightProfile& OutProfile)
{
    OutProfile = FTUHandlingSightProfile();
    if (!Mesh || !Mesh->GetStaticMesh()) return false;
    if (Mesh->GetStaticMesh()->GetPathName() != TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")) return false;

    // Geometry provenance: evidence/handling-rifle.obj, exported from the installed
    // Epic template on 2026-09-26. OBJ exports (X,Z,Y), converted here to mesh XYZ.
    // Rear is the midpoint of the two upper INSIDE notch lips, not the receiver
    // bounds. Front is the actual central post tip, not the taller outer guards.
    // Center of the measured inner aperture, not its upper rim. The earlier
    // rim calibration centered a number while placing the post above the opening.
    const FVector RearInMesh(-.012f,1.296f,16.096f);
    const FVector FrontInMesh(.001824, 34.363831, 16.717348);
    const FTransform MeshInActor = Mesh->GetOwner()
        ? Mesh->GetComponentTransform().GetRelativeTransform(Mesh->GetOwner()->GetActorTransform())
        : Mesh->GetRelativeTransform();
    const FVector Rear = MeshInActor.TransformPosition(RearInMesh);
    const FVector Front = MeshInActor.TransformPosition(FrontInMesh);
    const FVector Forward = (Front - Rear).GetSafeNormal();
    if (Forward.IsNearlyZero()) return false;
    const FQuat SightRotation = FRotationMatrix::MakeFromXZ(Forward,
        MeshInActor.TransformVectorNoScale(FVector::UpVector)).ToQuat();
    OutProfile.CalibrationId = TEXT("Epic_SM_Rifle_InnerAperture_20260926");
    OutProfile.SightInActor = FTransform(SightRotation, Rear);
    // Eye relief is an explicit presentation choice, not an asserted mesh socket.
    // The measured sight line remains unchanged at all eye relief distances/FOVs.
    constexpr float EyeReliefCm = 22.f;
    OutProfile.EyeInActor = FTransform(SightRotation, Rear - Forward * EyeReliefCm);
    OutProfile.FrontSightInActor = Front;
    OutProfile.bHasFrontSight = true;
    return true;
}
