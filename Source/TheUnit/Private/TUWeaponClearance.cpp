#include "TUWeaponClearance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

bool TUWeaponClearance::IsClear(UWorld* World,const AActor* Operator,const AActor* Weapon,
    const FTransform& Pose,const FBox& Bounds)
{
    if(!World || !World->GetPhysicsScene() || !Bounds.IsValid) return true;
    if(Pose.ContainsNaN()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TUPhysicalWeaponClearance),false);
    if(Operator) Query.AddIgnoredActor(Operator);
    if(Weapon) Query.AddIgnoredActor(Weapon);
    const FVector Extent=Bounds.GetExtent()*Pose.GetScale3D().GetAbs()+FVector(2.f);
    return !World->OverlapBlockingTestByChannel(Pose.TransformPosition(Bounds.GetCenter()),Pose.GetRotation(),
        ECC_Visibility,FCollisionShape::MakeBox(Extent),Query);
}
FTUWeaponClearanceResult TUWeaponClearance::Resolve(UWorld* World,const AActor* Operator,
    const AActor* Weapon,const FTransform& Desired,const FTransform& Eye,const FBox& Bounds,const FTransform& Previous)
{
    FTUWeaponClearanceResult R;R.Pose=Desired;
    if(IsClear(World,Operator,Weapon,Desired,Bounds)) return R;
    R.bDesiredBlocked=true;
    // These are physical world-space ready poses. We never shrink or hide a
    // viewmodel to allow an obstructed muzzle to fire through geometry.
    // Prefer compressed high-ready/side-ready poses that keep the firing grip
    // and receiver in the owner's frame. The old candidates translated the
    // complete weapon behind/below the eye, which was physically clear but
    // visually made wall reloads disappear.
    const FTransform Candidates[]={
        FTransform(FRotator(-28.f,38.f,-12.f),FVector(18.f,4.f,-13.f))*Eye,
        FTransform(FRotator(-32.f,-38.f,12.f),FVector(18.f,-4.f,-13.f))*Eye,
        FTransform(FRotator(-48.f,22.f,-8.f),FVector(12.f,4.f,-17.f))*Eye,
        FTransform(FRotator(-48.f,-22.f,8.f),FVector(12.f,-4.f,-17.f))*Eye,
        FTransform(FRotator(-62.f,0.f,0.f),FVector(8.f,0.f,-20.f))*Eye};
    for(const FTransform& Candidate:Candidates) {
        if(!IsClear(World,Operator,Weapon,Candidate,Bounds)) continue;
        // Find the closest safe portion of the approach instead of interpolating
        // through the obstacle. A conservative final query includes the magazine.
        R.Pose=Candidate;
        for(int32 Step=1;Step<=12;++Step) {
            FTransform Trial;Trial.Blend(Candidate,Desired,float(Step)/12.f);
            if(!IsClear(World,Operator,Weapon,Trial,Bounds)) break;
            R.Pose=Trial;
        }
        return R;
    }
    if(FVector::DistSquared(Previous.GetLocation(),Eye.GetLocation())<FMath::Square(100.f)
        && IsClear(World,Operator,Weapon,Previous,Bounds)) { R.Pose=Previous;return R; }
    R.bResolved=false;
    return R;
}
