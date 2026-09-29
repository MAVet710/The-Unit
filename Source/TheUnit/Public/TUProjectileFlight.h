#pragma once
#include "CoreMinimal.h"
/** Pure SI flight model. No actor, ammunition mutation, renderer or damage policy. */
namespace TUFlight
{
struct FDragPoint { double Mach=0.; double Cd=0.; };
struct FModel
{
    double MassKg=.004;
    double AreaM2=.0000246;
    TArray<FDragPoint> Drag;
    bool IsValid() const;
    double Coefficient(double Mach) const;
};
struct FEnvironment
{
    FVector GravityMps2=FVector(0,0,-9.80665);
    FVector WindMps=FVector::ZeroVector;
    double DensityKgM3=1.225;
    double SoundSpeedMps=343.;
    bool IsValid() const;
};
struct FState
{
    FVector PositionM=FVector::ZeroVector;
    FVector VelocityMps=FVector::ZeroVector;
    double AgeSeconds=0.;
    double DistanceM=0.;
    bool IsValid() const;
};
THEUNIT_API FVector Acceleration(const FVector& Velocity,const FModel& Model,const FEnvironment& Env);
/** One RK4 step; invalid input returns false without changing the state. */
THEUNIT_API bool Step(FState& State,double Seconds,const FModel& Model,const FEnvironment& Env);
THEUNIT_API double KineticEnergy(const FState& State,const FModel& Model);
/** Versioned provisional virtual projectile. This is NOT measured ammunition data. */
THEUNIT_API FModel PrototypeModel();
}
