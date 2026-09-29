#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TU_TacticalRifle.h"
#include "TU_WeaponBase.h"
#include "Components/BoxComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWorldMuzzleTest,"TheUnit.Combat.WorldMuzzleObstruction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUWorldMuzzleTest::RunTest(const FString& Parameters)
{
 const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!TestNotNull(TEXT("Physics world"),World)) return false;
 auto* Weapon=World->SpawnActor<ATU_WeaponBase>(); Weapon->SetActorLocation(FVector(0,0,100));
 auto* Cover=World->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(Cover); Cover->SetRootComponent(Box); Cover->AddInstanceComponent(Box);
 Box->SetBoxExtent(FVector(5,40,30)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent(); Cover->SetActorLocation(FVector(40,0,100));
 TestTrue(TEXT("Weapon path intersects low cover"),Weapon->IsMuzzleObstructed());
 const int32 Before=Weapon->GetCurrentAmmo(); Weapon->FireSingleShot(); TestEqual(TEXT("Obstruction prevents spending or firing through cover"),Weapon->GetCurrentAmmo(),Before);
 Cover->SetActorLocation(FVector(40,0,-100)); TestFalse(TEXT("Clear muzzle becomes ready"),Weapon->IsMuzzleObstructed());
 Weapon->FireSingleShot(); TestEqual(TEXT("Clear shot spends one"),Weapon->GetCurrentAmmo(),Before-1);
 World->DestroyWorld(false); return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponFrameRateTest,"TheUnit.Combat.FrameIndependentWeaponTimers",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTUWeaponFrameRateTest::RunTest(const FString& Parameters)
{
 if (!TestNotNull(TEXT("Engine"),GEngine)) return false;
 TGuardValue<uint64> FrameGuard(GFrameCounter,GFrameCounter);
 for (const int32 FPS : {30,60,144}) {
  const float Dt=1.f/FPS;
  const UWorld::InitializationValues Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
  UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
  if(!World) return false;
  GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
  auto Tick=[World,Dt](){ ++GFrameCounter; World->Tick(LEVELTICK_All,Dt); };
  auto* W=World->SpawnActor<ATU_TacticalRifle>();
  W->SetFireMode(ETUFireMode::FullAuto);
  const int32 Before=W->GetCurrentAmmo(); const float Start=World->GetTimeSeconds(); W->StartFire();
  for(int32 I=0;I<FPS*2;++I) { W->StartFire(); Tick(); }
  W->StopFire();
  const int32 Fired=Before-W->GetCurrentAmmo();
  const int32 Expected=1+FMath::FloorToInt((World->GetTimeSeconds()-Start)/W->GetFireIntervalSeconds());
  TestTrue(FString::Printf(TEXT("%d FPS absolute cadence fired %d, expected %d +/- one boundary shot"),FPS,Fired,Expected),FMath::Abs(Fired-Expected)<=1);
  W->StartReload(); const float ReloadStart=World->GetTimeSeconds(); const float PhaseDuration=W->GetActionState().PhaseEndServerTime-ReloadStart;
  int32 Frames=0; while(W->IsReloading() && Frames++<FPS*4) Tick();
  const float Elapsed=World->GetTimeSeconds()-ReloadStart;
  TestFalse(TEXT("Real timers reach Ready"),W->IsReloading());
  TestTrue(FString::Printf(TEXT("%d FPS reload elapsed %.5f expected %.5f within one frame"),FPS,Elapsed,5.f*PhaseDuration),Elapsed+KINDA_SMALL_NUMBER>=5.f*PhaseDuration && Elapsed<=5.f*PhaseDuration+Dt+0.002f);
  auto* Burst=World->SpawnActor<ATU_WeaponBase>(); Burst->EnableTimedCadenceForTesting(); Burst->SetFireMode(ETUFireMode::Burst);
  const int32 BurstBefore=Burst->GetCurrentAmmo(); Burst->StartFire();
  for(int32 I=0;I<100;++I) Burst->StartFire();
  TestEqual(TEXT("Repeated same-frame burst intent fires one initial shot"),Burst->GetCurrentAmmo(),BurstBefore-1);
  for(int32 I=0;I<FPS;++I) Tick();
  TestEqual(TEXT("Repeated burst intent preserves exactly three scheduled shots"),Burst->GetCurrentAmmo(),BurstBefore-3);
  GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 }
 return true;
}
#endif