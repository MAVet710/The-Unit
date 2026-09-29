#include "TUProjectileWorldSubsystem.h"
#include "TU_WeaponBase.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
namespace { constexpr double FlightStep=1./240.; constexpr int32 MaxActive=1024,MaxStepsPerTick=64; }
bool UTUProjectileWorldSubsystem::DoesSupportWorldType(EWorldType::Type Type) const { return Type==EWorldType::Game||Type==EWorldType::PIE; }
TStatId UTUProjectileWorldSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UTUProjectileWorldSubsystem,STATGROUP_Tickables); }
void UTUProjectileWorldSubsystem::Deinitialize(){Active.Reset();AcceptedIds.Reset();OnImpact.Clear();Super::Deinitialize();}
void UTUProjectileWorldSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if(GetWorld()&&GetWorld()->GetNetMode()!=NM_Client)AdvanceTo(GetWorld()->GetTimeSeconds());
}
bool UTUProjectileWorldSubsystem::CanAcceptLaunch(const FTUProjectileLaunch& S) const
{
    return GetWorld()&&GetWorld()->GetPhysicsScene()&&GetWorld()->GetNetMode()!=NM_Client&&!bAdvancing
        &&S.ShotId.IsValid()&&!AcceptedIds.Contains(S.ShotId)&&Active.Num()<MaxActive&&AcceptedIds.Num()<1000000
        &&S.Initial.IsValid()&&S.Initial.AgeSeconds==0.&&S.Initial.DistanceM==0.&&S.Model.IsValid()&&S.Environment.IsValid()
        &&FMath::IsFinite(S.ServerTime)&&S.ServerTime>=0.&&(!bClockStarted||S.ServerTime>=Clock-1.e-7)
        &&FMath::IsFinite(S.MaxDistanceM)&&S.MaxDistanceM>0.&&S.MaxDistanceM<=100000.
        &&FMath::IsFinite(S.MaxAgeSeconds)&&S.MaxAgeSeconds>0.&&S.MaxAgeSeconds<=120.
        &&FMath::IsFinite(S.RadiusM)&&S.RadiusM>=0.&&S.RadiusM<=.1
        &&FMath::IsFinite(S.GameplayDamage)&&S.GameplayDamage>=0.f&&S.Initial.VelocityMps.Size()<=10000.;
}
bool UTUProjectileWorldSubsystem::TryLaunch(const FTUProjectileLaunch& Shot,TFunctionRef<bool()> CommitAmmunition)
{
    if(!CanAcceptLaunch(Shot))return false;
    TGuardValue<bool> Guard(bAdvancing,true);
    if(!CommitAmmunition())return false;
    if(!bClockStarted||Active.IsEmpty()){Clock=Shot.ServerTime;bClockStarted=true;}
    AcceptedIds.Add(Shot.ShotId);Active.Add({Shot,Shot.Initial});return true;
}
bool UTUProjectileWorldSubsystem::ReadFlight(const FGuid& Id,TUFlight::FState& Out) const
{
    for(const auto& P:Active)if(P.Launch.ShotId==Id){Out=P.State;return true;}return false;
}
void UTUProjectileWorldSubsystem::AdvanceTo(double Target)
{
    if(!GetWorld()||GetWorld()->GetNetMode()==NM_Client||bAdvancing||!FMath::IsFinite(Target)||Target<0.)return;
    if(!bClockStarted||Active.IsEmpty()){Clock=Target;bClockStarted=true;PendingBacklog=0.;return;}
    if(Target<Clock)return;
    TGuardValue<bool> Guard(bAdvancing,true);
    struct FPending { FTUProjectileLaunch Launch; FTUProjectileImpact Impact; };
    for(int32 StepIndex=0;StepIndex<MaxStepsPerTick&&Clock+FlightStep<=Target+1.e-9;++StepIndex)
    {
        const double Until=Clock+FlightStep;TArray<FPending> Hits;
        for(int32 I=Active.Num()-1;I>=0;--I)
        {
            auto& P=Active[I];const double Start=P.Launch.ServerTime+P.State.AgeSeconds;
            if(Start>=Until-1.e-10)continue;
            double H=FMath::Min(Until-Start,P.Launch.MaxAgeSeconds-P.State.AgeSeconds);
            if(H<=1.e-10||P.State.DistanceM>=P.Launch.MaxDistanceM){Active.RemoveAtSwap(I);continue;}
            TUFlight::FState Next=P.State;
            if(!TUFlight::Step(Next,H,P.Launch.Model,P.Launch.Environment))
            {++NumericalFailures;UE_LOG(LogTemp,Error,TEXT("TU_FLIGHT invalid integration shot=%s"),*P.Launch.ShotId.ToString());Active.RemoveAtSwap(I);continue;}
            if(Next.DistanceM>P.Launch.MaxDistanceM)
            {
                double Low=0.,High=H;
                for(int32 J=0;J<18;++J){const double Mid=(Low+High)*.5;auto Trial=P.State;TUFlight::Step(Trial,Mid,P.Launch.Model,P.Launch.Environment);if(Trial.DistanceM<P.Launch.MaxDistanceM)Low=Mid;else High=Mid;}
                H=High;Next=P.State;TUFlight::Step(Next,H,P.Launch.Model,P.Launch.Environment);
            }
            FCollisionQueryParams Query(SCENE_QUERY_STAT(TUProjectileFlightSweep),true);
            if(P.Launch.Causer.IsValid())Query.AddIgnoredActor(P.Launch.Causer.Get());
            if(P.Launch.Shooter.IsValid())Query.AddIgnoredActor(P.Launch.Shooter.Get());
            FHitResult Hit;const FVector From=P.State.PositionM*100.,To=Next.PositionM*100.;
            const bool Blocked=P.Launch.RadiusM>0.?
                GetWorld()->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(P.Launch.RadiusM*100.),Query):
                GetWorld()->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,Query);
            if(Blocked)
            {
                auto AtImpact=P.State;const double Partial=H*FMath::Clamp(double(Hit.Time),0.,1.);
                if(Partial>0.)TUFlight::Step(AtImpact,Partial,P.Launch.Model,P.Launch.Environment);
                FTUProjectileImpact E;E.ShotId=P.Launch.ShotId;E.WeaponId=P.Launch.WeaponId;E.AmmoId=P.Launch.AmmoId;
                E.Hit=Hit;E.VelocityMps=AtImpact.VelocityMps;E.FlightSeconds=AtImpact.AgeSeconds;E.EnergyJoules=TUFlight::KineticEnergy(AtImpact,P.Launch.Model);
                Hits.Add({P.Launch,E});Active.RemoveAtSwap(I);
            }
            else {P.State=Next;if(P.State.AgeSeconds>=P.Launch.MaxAgeSeconds-1.e-9||P.State.DistanceM>=P.Launch.MaxDistanceM-1.e-6)Active.RemoveAtSwap(I);}
        }
        Clock=Until;
        Hits.StableSort([](const FPending& A,const FPending& B){return A.Launch.ServerTime+A.Impact.FlightSeconds<B.Launch.ServerTime+B.Impact.FlightSeconds;});
        for(const auto& Pending:Hits)
        {
            const auto& E=Pending.Impact;++TotalImpacts;
            // Existing game damage policy remains separate from energy telemetry.
            if(AActor* TargetActor=E.Hit.GetActor())
                UGameplayStatics::ApplyPointDamage(TargetActor,Pending.Launch.GameplayDamage,E.VelocityMps.GetSafeNormal(),E.Hit,
                    Pending.Launch.Controller.Get(),Pending.Launch.Causer.Get(),UDamageType::StaticClass());
            if(auto* Weapon=Cast<ATU_WeaponBase>(Pending.Launch.Causer.Get()))
                Weapon->ReportProjectileImpact(E);
            OnImpact.Broadcast(E);
            UE_LOG(LogTemp,Verbose,TEXT("TU_FLIGHT impact id=%s age=%.6f energy=%.3f"),*E.ShotId.ToString(),E.FlightSeconds,E.EnergyJoules);
        }
    }
    PendingBacklog=FMath::Max(0.,Target-Clock);
    // An overloaded frame retains elapsed time. It never becomes a hitscan shot.
}
