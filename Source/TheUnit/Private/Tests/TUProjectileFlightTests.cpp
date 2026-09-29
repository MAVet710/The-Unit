#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TUProjectileFlight.h"
#include "TUProjectileWorldSubsystem.h"
#include "TU_TacticalRifle.h"
#include "TU_WeaponBase.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/BoxComponent.h"
#include <limits>
namespace TUFlightTests
{
struct FWorld
{
    UWorld* Value=nullptr;
    FWorld(){const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        Value=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);if(Value&&GEngine)GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Value);}
    ~FWorld(){if(Value){if(GEngine)GEngine->DestroyWorldContext(Value);Value->DestroyWorld(false);}}
};
static AActor* Box(UWorld* World,double X,double Thickness=.05)
{
    auto* A=World->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);A->AddInstanceComponent(B);
    B->SetBoxExtent(FVector(Thickness,100.,100.));B->SetCollisionEnabled(ECollisionEnabled::QueryOnly);B->SetCollisionObjectType(ECC_WorldStatic);
    B->SetCollisionResponseToAllChannels(ECR_Ignore);B->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);B->RegisterComponent();A->SetActorLocation(FVector(X,0,0));return A;
}
static FTUProjectileLaunch Shot(){FTUProjectileLaunch S;S.ShotId=FGuid::NewGuid();S.WeaponId=FGuid::NewGuid();S.Initial.VelocityMps=FVector(100,0,0);S.Environment.DensityKgM3=0.;S.Environment.GravityMps2=FVector::ZeroVector;S.RadiusM=0.;return S;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFlightVacuum,"TheUnit.Flight.AnalyticVacuum",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUFlightVacuum::RunTest(const FString&)
{
    auto M=TUFlight::PrototypeModel();TUFlight::FEnvironment E;E.DensityKgM3=0.;
    TUFlight::FState S;S.PositionM=FVector(1,2,3);S.VelocityMps=FVector(100,10,20);
    for(int32 I=0;I<480;++I)TestTrue(TEXT("Valid SI step"),TUFlight::Step(S,1./240.,M,E));
    const FVector Expected=FVector(1,2,3)+FVector(100,10,20)*2.+E.GravityMps2*2.;
    TestTrue(TEXT("Vacuum position matches closed form in metres"),S.PositionM.Equals(Expected,1.e-8));
    TestTrue(TEXT("Vacuum velocity matches closed form"),S.VelocityMps.Equals(FVector(100,10,20)+E.GravityMps2*2.,1.e-8));
    TestTrue(TEXT("Age is physical time"),FMath::IsNearlyEqual(S.AgeSeconds,2.,1.e-10));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFlightDrag,"TheUnit.Flight.AnalyticQuadraticDrag",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUFlightDrag::RunTest(const FString&)
{
    TUFlight::FModel M;M.MassKg=2.;M.AreaM2=.1;M.Drag={{0.,.7},{5.,.7}};
    TUFlight::FEnvironment E;E.DensityKgM3=1.1;E.GravityMps2=FVector::ZeroVector;
    TUFlight::FState S;S.VelocityMps=FVector(80,0,0);double Previous=TUFlight::KineticEnergy(S,M);
    for(int32 I=0;I<480;++I){TestTrue(TEXT("Drag step"),TUFlight::Step(S,1./240.,M,E));const double Energy=TUFlight::KineticEnergy(S,M);TestTrue(TEXT("Stationary-air drag does not add energy"),Energy<=Previous+1.e-9);Previous=Energy;}
    const double K=E.DensityKgM3*.7*M.AreaM2/(2.*M.MassKg);
    TestTrue(TEXT("Velocity agrees with independent quadratic-drag solution"),FMath::Abs(S.VelocityMps.X-80./(1.+K*80.*2.))<1.e-6);
    TestTrue(TEXT("Position agrees with log closed form"),FMath::Abs(S.PositionM.X-FMath::Loge(1.+K*80.*2.)/K)<1.e-6);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFlightWind,"TheUnit.Flight.WindFrameAndValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUFlightWind::RunTest(const FString&)
{
    auto M=TUFlight::PrototypeModel();TUFlight::FEnvironment E,W;W.WindMps=FVector(7,-3,2);
    TUFlight::FState A,B;A.VelocityMps=FVector(200,30,50);B=A;B.VelocityMps+=W.WindMps;
    for(int32 I=0;I<240;++I){TUFlight::Step(A,1./240.,M,E);TUFlight::Step(B,1./240.,M,W);}
    TestTrue(TEXT("Wind force uses air-relative velocity"),B.VelocityMps.Equals(A.VelocityMps+W.WindMps,1.e-8));
    TestTrue(TEXT("Galilean position offset agrees"),B.PositionM.Equals(A.PositionM+W.WindMps,1.e-8));
    TestTrue(TEXT("Mach coefficient interpolates declared table"),FMath::IsNearlyEqual(M.Coefficient(.9),.29,1.e-10));
    auto Before=A;TestFalse(TEXT("Negative step rejected"),TUFlight::Step(A,-1.,M,E));
    E.DensityKgM3=-1.;TestFalse(TEXT("Negative density rejected"),TUFlight::Step(A,.01,M,E));E.DensityKgM3=1.;
    M.MassKg=0.;TestFalse(TEXT("Zero mass rejected"),TUFlight::Step(A,.01,M,E));M.MassKg=.004;
    M.Drag[1].Mach=0.;TestFalse(TEXT("Ambiguous drag knots rejected"),TUFlight::Step(A,.01,M,E));M=TUFlight::PrototypeModel();
    TestFalse(TEXT("NaN step rejected"),TUFlight::Step(A,std::numeric_limits<double>::quiet_NaN(),M,E));
    TestTrue(TEXT("Invalid calls never mutate state"),A.PositionM==Before.PositionM&&A.VelocityMps==Before.VelocityMps);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFlightSweeps,"TheUnit.Flight.ThinFirstImpactExactlyOnce",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUFlightSweeps::RunTest(const FString&)
{
    TUFlightTests::FWorld W;if(!TestNotNull(TEXT("World"),W.Value))return false;
    auto* F=W.Value->GetSubsystem<UTUProjectileWorldSubsystem>();if(!TestNotNull(TEXT("World flight subsystem"),F))return false;
    auto* First=TUFlightTests::Box(W.Value,10000.);TUFlightTests::Box(W.Value,11000.);
    int32 Spent=0,Events=0;FTUProjectileImpact Result;F->OnImpact.AddLambda([&](const FTUProjectileImpact& E){++Events;Result=E;});
    auto Shot=TUFlightTests::Shot();TestTrue(TEXT("Admit shot"),F->TryLaunch(Shot,[&]{++Spent;return true;}));
    TestFalse(TEXT("Duplicate shot cannot spend twice"),F->TryLaunch(Shot,[&]{++Spent;return true;}));
    for(int32 I=1;I<=5;++I)F->AdvanceTo(I*.1);TestEqual(TEXT("Distant target is not hit instantaneously"),Events,0);
    for(int32 I=6;I<=12;++I)F->AdvanceTo(I*.1);
    TestEqual(TEXT("Thin surface receives one impact"),Events,1);TestEqual(TEXT("Nearest surface blocks flight"),Result.Hit.GetActor(),First);
    TestTrue(TEXT("One-second travel time measured"),FMath::Abs(Result.FlightSeconds-.999995)<.0001);
    F->AdvanceTo(2.);F->AdvanceTo(2.);TestEqual(TEXT("Repeated update cannot duplicate damage event"),Events,1);
    TestEqual(TEXT("One accepted shot spent one cartridge"),Spent,1);TestEqual(TEXT("Opaque hit retires flight"),F->ActiveCount(),0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUFlightOwnership,"TheUnit.Flight.LifetimeBirthAndAdmission",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUFlightOwnership::RunTest(const FString&)
{
    TUFlightTests::FWorld W;if(!W.Value)return false;auto* F=W.Value->GetSubsystem<UTUProjectileWorldSubsystem>();if(!F)return false;
    auto Shot=TUFlightTests::Shot();Shot.ServerTime=.2;Shot.Causer=W.Value->SpawnActor<AActor>();
    TestTrue(TEXT("World owns launched projectile"),F->TryLaunch(Shot,[]{return true;}));Shot.Causer->Destroy();
    F->AdvanceTo(.1);TUFlight::FState State;TestTrue(TEXT("Projectile survives causer destruction"),F->ReadFlight(Shot.ShotId,State));
    TestEqual(TEXT("No motion before recorded birth"),State.AgeSeconds,0.);
    F->AdvanceTo(.3);F->ReadFlight(Shot.ShotId,State);TestTrue(TEXT("Age excludes time before birth"),FMath::Abs(State.AgeSeconds-.1)<1.e-7);
    auto Denied=TUFlightTests::Shot();Denied.ServerTime=.3;TestFalse(TEXT("Failed ammunition commit does not launch"),F->TryLaunch(Denied,[]{return false;}));
    TestEqual(TEXT("No ghost projectile"),F->ActiveCount(),1);
    int32 Spent=0;for(int32 I=1;I<1024;++I){auto S=TUFlightTests::Shot();S.ServerTime=.3;TestTrue(TEXT("Capacity reservation"),F->TryLaunch(S,[&]{++Spent;return true;}));}
    auto Full=TUFlightTests::Shot();Full.ServerTime=.3;TestFalse(TEXT("Capacity denial spends nothing"),F->TryLaunch(Full,[&]{++Spent;return true;}));
    TestEqual(TEXT("Exact admitted spend count"),Spent,1023);return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
