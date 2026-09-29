#pragma once
#include "CoreMinimal.h"
class UWorld;
class AActor;
struct THEUNIT_API FTUWeaponClearanceResult
{
    FTransform Pose=FTransform::Identity;
    bool bDesiredBlocked=false;
    bool bResolved=true;
};
namespace TUWeaponClearance
{
    THEUNIT_API bool IsClear(UWorld* World,const AActor* Operator,const AActor* Weapon,
        const FTransform& Pose,const FBox& LocalBounds);
    THEUNIT_API FTUWeaponClearanceResult Resolve(UWorld* World,const AActor* Operator,
        const AActor* Weapon,const FTransform& Desired,const FTransform& Eye,
        const FBox& LocalBounds,const FTransform& Previous);
}
