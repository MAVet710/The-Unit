#include "TUProjectileFlight.h"
namespace TUFlight
{
bool FModel::IsValid() const
{
    if(!FMath::IsFinite(MassKg)||MassKg<=0.||!FMath::IsFinite(AreaM2)||AreaM2<=0.||Drag.IsEmpty())return false;
    double Last=-1.;
    for(const auto& P:Drag){if(!FMath::IsFinite(P.Mach)||P.Mach<0.||P.Mach<=Last||!FMath::IsFinite(P.Cd)||P.Cd<0.)return false;Last=P.Mach;}
    return true;
}
double FModel::Coefficient(double Mach) const
{
    if(Drag.IsEmpty()||!FMath::IsFinite(Mach))return 0.;
    if(Mach<=Drag[0].Mach)return Drag[0].Cd;
    for(int32 I=1;I<Drag.Num();++I)if(Mach<=Drag[I].Mach)
        return FMath::Lerp(Drag[I-1].Cd,Drag[I].Cd,(Mach-Drag[I-1].Mach)/(Drag[I].Mach-Drag[I-1].Mach));
    return Drag.Last().Cd;
}
bool FEnvironment::IsValid() const
{
    return !GravityMps2.ContainsNaN()&&!WindMps.ContainsNaN()&&FMath::IsFinite(DensityKgM3)&&DensityKgM3>=0.&&FMath::IsFinite(SoundSpeedMps)&&SoundSpeedMps>0.;
}
bool FState::IsValid() const
{
    return !PositionM.ContainsNaN()&&!VelocityMps.ContainsNaN()&&FMath::IsFinite(AgeSeconds)&&AgeSeconds>=0.&&FMath::IsFinite(DistanceM)&&DistanceM>=0.;
}
FVector Acceleration(const FVector& Velocity,const FModel& Model,const FEnvironment& Env)
{
    const FVector Air=Velocity-Env.WindMps;const double Speed=Air.Size();
    return Env.GravityMps2-Air*(Env.DensityKgM3*Model.Coefficient(Speed/Env.SoundSpeedMps)*Model.AreaM2*Speed/(2.*Model.MassKg));
}
bool Step(FState& State,double H,const FModel& Model,const FEnvironment& Env)
{
    if(!State.IsValid()||!Model.IsValid()||!Env.IsValid()||!FMath::IsFinite(H)||H<=0.||H>.05)return false;
    const FVector V1=State.VelocityMps;
    const FVector A1=Acceleration(V1,Model,Env);
    const FVector V2=V1+A1*(H*.5),A2=Acceleration(V2,Model,Env);
    const FVector V3=V1+A2*(H*.5),A3=Acceleration(V3,Model,Env);
    const FVector V4=V1+A3*H,A4=Acceleration(V4,Model,Env);
    FState Next=State;
    Next.PositionM+=(V1+2.*V2+2.*V3+V4)*(H/6.);
    Next.VelocityMps+=(A1+2.*A2+2.*A3+A4)*(H/6.);
    Next.DistanceM+=(V1.Size()+2.*V2.Size()+2.*V3.Size()+V4.Size())*(H/6.);
    Next.AgeSeconds+=H;
    if(!Next.IsValid())return false;
    State=Next;return true;
}
double KineticEnergy(const FState& State,const FModel& Model)
{
    return .5*Model.MassKg*State.VelocityMps.SizeSquared();
}
FModel PrototypeModel()
{
    FModel M;
    // Engineering curve TU-virtual-v1. No manufacturer/Doppler calibration claimed.
    M.Drag={{0.,.20},{.8,.20},{1.,.38},{1.2,.40},{2.,.30},{3.,.25},{5.,.23}};
    return M;
}
}
