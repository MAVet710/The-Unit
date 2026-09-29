#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TUProjectileFlight.h"
#include "TUProjectileWorldSubsystem.generated.h"
class AController;
struct FTUProjectileLaunch
{
    FGuid ShotId;
    FGuid WeaponId;
    FName AmmoId;
    TUFlight::FState Initial;
    TUFlight::FModel Model=TUFlight::PrototypeModel();
    TUFlight::FEnvironment Environment;
    TWeakObjectPtr<AActor> Shooter;
    TWeakObjectPtr<AActor> Causer;
    TWeakObjectPtr<AController> Controller;
    double ServerTime=0.;
    double MaxDistanceM=1200.;
    double MaxAgeSeconds=10.;
    double RadiusM=.0028;
    float GameplayDamage=0.f;
};
struct FTUProjectileImpact
{
    FGuid ShotId; FGuid WeaponId; FName AmmoId;
    FHitResult Hit;
    FVector VelocityMps=FVector::ZeroVector;
    double FlightSeconds=0.; double EnergyJoules=0.;
};
DECLARE_MULTICAST_DELEGATE_OneParam(FTUFlightImpactEvent,const FTUProjectileImpact&);
/** Server-only projectile lifetime, independent of the currently equipped weapon. */
UCLASS()
class THEUNIT_API UTUProjectileWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;
    bool CanAcceptLaunch(const FTUProjectileLaunch& Shot) const;
    bool TryLaunch(const FTUProjectileLaunch& Shot,TFunctionRef<bool()> CommitAmmunition);
    void AdvanceTo(double ServerTime);
    int32 ActiveCount() const { return Active.Num(); }
    uint64 ImpactCount() const { return TotalImpacts; }
    uint64 NumericalFailureCount() const { return NumericalFailures; }
    double BacklogSeconds() const { return PendingBacklog; }
    bool ReadFlight(const FGuid& Id,TUFlight::FState& Out) const;
    FTUFlightImpactEvent OnImpact;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
    struct FLive { FTUProjectileLaunch Launch; TUFlight::FState State; };
    TArray<FLive> Active;
    TSet<FGuid> AcceptedIds;
    double Clock=0.,PendingBacklog=0.;
    uint64 TotalImpacts=0,NumericalFailures=0;
    bool bClockStarted=false,bAdvancing=false;
};
