#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TU_WeaponBase.h"
#include "TUWorldItem.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"

namespace TUWeaponTest
{
static UWorld* World()
{
 const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 UWorld* W = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if (W && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
 return W;
}
static FTUItemLedger Kit(ATU_WeaponBase* W)
{
 auto L = W->ExportItemLedger(); L.Magazines.SetNum(2); L.Magazines[0].Cartridges.Init(TEXT("Ammo_556_Training_Ball"),30); L.Magazines[1].Cartridges.Init(TEXT("Ammo_556_Training_Ball"),30); return L;
}
static int32 Count(const FTUItemLedger& L)
{
 int32 N = L.LooseCartridges.Num(); for(const auto& M:L.Magazines) N += M.Cartridges.Num(); for(const auto& W:L.Weapons) N += !W.ChamberAmmoId.IsNone(); return N;
}
static void Finish(ATU_WeaponBase* W) { for(int32 I=0; I<5 && W->GetActionState().bActive; ++I) { auto A=W->GetActionState(); W->AdvanceActionClockForTesting(); } }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponOwnershipTest,"TheUnit.Combat.WeaponOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUWeaponOwnershipTest::RunTest(const FString& Parameters)
{
 UWorld* World=TUWeaponTest::World(); if(!TestNotNull(TEXT("World"),World)) return false;
 auto* W=World->SpawnActor<ATU_WeaponBase>(); auto L=TUWeaponTest::Kit(W);
 TestTrue(TEXT("Import physical 61-round kit"),W->ImportItemLedger(L));
 TestEqual(TEXT("61-round initial total"),TUWeaponTest::Count(W->ExportItemLedger()),61);
 for(int32 I=0;I<11;++I) W->FireSingleShot();
 TestEqual(TEXT("Eleven successful shots spend eleven"),TUWeaponTest::Count(W->ExportItemLedger()),50);
 TestEqual(TEXT("19 remain in inserted magazine"),W->GetMagazineState().RoundsInMagazine,19);
 const FGuid Original=L.Magazines[0].InstanceId;
 const FGuid Action=FGuid::NewGuid(); TestTrue(TEXT("Begin retained reload"),W->RequestReload(Action,W->ExportItemLedger().Revision,ETUReloadPolicy::Retain));
 TUWeaponTest::Finish(W); L=W->ExportItemLedger();
 TestEqual(TEXT("Retained reload conserves 50"),TUWeaponTest::Count(L),50);
 TestEqual(TEXT("30 inserted plus loaded chamber"),W->GetCurrentAmmo(),31);
 const auto* Old=L.Magazines.FindByPredicate([Original](const auto& M){return M.InstanceId==Original;});
 TestTrue(TEXT("Original magazine retains 19"),Old && Old->Cartridges.Num()==19 && Old->Location==ETUItemLocation::Carried);
 const int32 Revision=L.Revision; W->Inspect(); TestEqual(TEXT("Inspection does not mutate"),W->ExportItemLedger().Revision,Revision);
 TestFalse(TEXT("Completed action ID cannot replay"),W->RequestReload(Action,Revision,ETUReloadPolicy::Retain));
 TestFalse(TEXT("Stale request denied"),W->RequestReload(FGuid::NewGuid(),Revision-1,ETUReloadPolicy::Retain));
 auto* Restored=World->SpawnActor<ATU_WeaponBase>(); TestTrue(TEXT("Restore durable contents"),Restored->ImportItemLedger(L));
 TestEqual(TEXT("Restore identity"),Restored->GetWeaponInstanceId(),W->GetWeaponInstanceId()); TestEqual(TEXT("Restore no refill"),TUWeaponTest::Count(Restored->ExportItemLedger()),50);
 if (GEngine) GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponActionTest,"TheUnit.Combat.WeaponActions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUWeaponActionTest::RunTest(const FString& Parameters)
{
 UWorld* World=TUWeaponTest::World(); if(!World) return false;
 for(int32 Boundary=0;Boundary<5;++Boundary) {
  auto* W=World->SpawnActor<ATU_WeaponBase>(); W->ImportItemLedger(TUWeaponTest::Kit(W));
  W->StartReload();
  const auto Started=W->GetActionState(); TestFalse(TEXT("Production notify cannot skip phase time"),W->CommitActionPhase(Started.ActionId,Started.Revision));
  for(int32 I=0;I<Boundary;++I) { const auto A=W->GetActionState(); TestTrue(TEXT("Commit exact phase"),W->AdvanceActionClockForTesting()); TestFalse(TEXT("Duplicate phase harmless"),W->CommitActionPhase(A.ActionId,A.Revision)); }
  const auto Prior=W->GetActionState(); W->InterruptWeaponAction();
  TestFalse(TEXT("Interrupted callback denied"),W->CommitActionPhase(Prior.ActionId,Prior.Revision));
  TestEqual(TEXT("Interruption conserves every boundary"),TUWeaponTest::Count(W->ExportItemLedger()),61);
  auto* Restore=World->SpawnActor<ATU_WeaponBase>(); TestTrue(TEXT("Save phase import"),Restore->ImportItemLedger(W->ExportItemLedger()));
  TestEqual(TEXT("Phase survives import"),Restore->GetActionState().Phase,Prior.Phase);
  TestTrue(TEXT("Resume committed phase"),Restore->RequestReload(FGuid::NewGuid(),Restore->ExportItemLedger().Revision,ETUReloadPolicy::Retain)); TUWeaponTest::Finish(Restore);
  TestEqual(TEXT("Resume never mints"),TUWeaponTest::Count(Restore->ExportItemLedger()),61);
 }
 auto* W=World->SpawnActor<ATU_WeaponBase>(); auto L=TUWeaponTest::Kit(W); L.Weapons[0].ChamberAmmoId=NAME_None; L.Weapons[0].bActionOpen=true;
 L.Magazines[0].Cartridges={FName(TEXT("Ammo_556_Training_Ball")),FName(TEXT("Ammo_556_Training_Tracer")),FName(TEXT("Ammo_556_Training_Subsonic"))}; TestTrue(TEXT("Mixed import"),W->ImportItemLedger(L));
 TestFalse(TEXT("Unchambered cannot fire"),W->CanFire()); W->FireSingleShot(); TestEqual(TEXT("No prefire feed"),W->GetMagazineState().RoundsInMagazine,3);
 TestTrue(TEXT("Manual cycle"),W->CycleAction()); TestEqual(TEXT("Feeds first cartridge"),W->ExportItemLedger().Weapons[0].ChamberAmmoId,FName(TEXT("Ammo_556_Training_Ball")));
 W->FireSingleShot(); TestEqual(TEXT("Self loading preserves ordered next"),W->ExportItemLedger().Weapons[0].ChamberAmmoId,FName(TEXT("Ammo_556_Training_Tracer")));
 W->CycleAction(); TestEqual(TEXT("Cycle retains ejected live round"),W->ExportItemLedger().LooseCartridges[0],FName(TEXT("Ammo_556_Training_Tracer")));
 const int32 Total=TUWeaponTest::Count(W->ExportItemLedger()); const FGuid Outgoing=W->ExportItemLedger().Weapons[0].InsertedMagazineId;
 W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Drop); auto A=W->GetActionState(); W->AdvanceActionClockForTesting();
 ATUWorldItem* Ground=nullptr; for(TActorIterator<ATUWorldItem> It(World);It;++It) if(It->GetMagazine().InstanceId==Outgoing) Ground=*It;
 TestNotNull(TEXT("Drop creates authoritative ground record"),Ground);
 if(Ground) { TestEqual(TEXT("Drop conserves across locations"),TUWeaponTest::Count(W->ExportItemLedger())+Ground->GetMagazine().Cartridges.Num(),Total); auto* Other=World->SpawnActor<ATU_WeaponBase>(); TestTrue(TEXT("First claimant succeeds"),Ground->TryPickup(Other)); TestFalse(TEXT("Second claimant denied"),Ground->TryPickup(W)); }
 W->InterruptWeaponAction(); W->Destroy(); if (GEngine) GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUEmptyAndDroppedReloadTest,"TheUnit.Combat.EmptyAndDroppedReloadBoundaries",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTUEmptyAndDroppedReloadTest::RunTest(const FString& Parameters)
{
 UWorld* World=TUWeaponTest::World(); if(!World) return false;
 auto* Empty=World->SpawnActor<ATU_WeaponBase>(); Empty->ImportItemLedger(TUWeaponTest::Kit(Empty));
 for(int32 I=0;I<31;++I) Empty->FireSingleShot();
 TestEqual(TEXT("Exhaustion spends all loaded ammunition"),Empty->GetCurrentAmmo(),0);
 TestTrue(TEXT("Empty self loader locks open"),Empty->ExportItemLedger().Weapons[0].bActionOpen);
 TestFalse(TEXT("Empty cannot fire"),Empty->CanFire()); Empty->StartReload();
 for(int32 Phase=0;Phase<3;++Phase) {
  TestFalse(TEXT("Not ready before committed insertion"),Empty->CanFire());
  Empty->AdvanceActionClockForTesting();
 }
 TestEqual(TEXT("Insertion preserves all 30 in magazine"),Empty->GetMagazineState().RoundsInMagazine,30);
 TestFalse(TEXT("Insertion alone does not silently chamber"),Empty->GetMagazineState().bRoundChambered);
 Empty->AdvanceActionClockForTesting();
 TestTrue(TEXT("Chamber phase feeds one"),Empty->GetMagazineState().bRoundChambered);
 TestEqual(TEXT("Chamber feed removes exactly one magazine cartridge"),Empty->GetMagazineState().RoundsInMagazine,29);
 TestFalse(TEXT("Chamber phase still awaits Ready"),Empty->CanFire());
 Empty->AdvanceActionClockForTesting(); TestTrue(TEXT("Ready permits firing after empty reload"),Empty->CanFire());
 TestEqual(TEXT("Empty reload conserves remaining 30"),TUWeaponTest::Count(Empty->ExportItemLedger()),30);
 TestFalse(TEXT("Empty carried magazines cannot create a replacement"),Empty->RequestReload(FGuid::NewGuid(),Empty->ExportItemLedger().Revision,ETUReloadPolicy::Retain));
 Empty->Destroy();
 for(int32 Boundary=0;Boundary<5;++Boundary) {
  auto* W=World->SpawnActor<ATU_WeaponBase>(); W->ImportItemLedger(TUWeaponTest::Kit(W));
  const FGuid Outgoing=W->ExportItemLedger().Weapons[0].InsertedMagazineId;
  W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Drop);
  for(int32 I=0;I<Boundary;++I) W->AdvanceActionClockForTesting();
  const auto OldAction=W->GetActionState(); W->InterruptWeaponAction();
  auto Saved=W->ExportItemLedger(); W->Destroy();
  auto* Restored=World->SpawnActor<ATU_WeaponBase>();
  TestTrue(TEXT("Dropped interruption state imports"),Restored->ImportItemLedger(Saved));
  TestTrue(TEXT("Dropped action resumes current phase"),Restored->RequestReload(FGuid::NewGuid(),Restored->ExportItemLedger().Revision,ETUReloadPolicy::Drop));
  TestFalse(TEXT("Prior action cannot advance resumed phase"),Restored->CommitActionPhase(OldAction.ActionId,OldAction.Revision));
  auto Current=Restored->GetActionState();
  TestFalse(TEXT("Future revision is rejected"),Restored->CommitActionPhase(Current.ActionId,Current.Revision+100));
  TestFalse(TEXT("Wrong action is rejected"),Restored->CommitActionPhase(FGuid::NewGuid(),Current.Revision));
  TUWeaponTest::Finish(Restored);
  int32 GroundCount=0,GroundRounds=0;
  for(TActorIterator<ATUWorldItem> It(World);It;++It) if(It->GetMagazine().InstanceId==Outgoing) { ++GroundCount; GroundRounds+=It->GetMagazine().Cartridges.Num(); }
  TestEqual(TEXT("Exactly one outgoing magazine exists on ground after resume"),GroundCount,1);
  TestEqual(TEXT("Drop interruption conserves 61 across actor and ground"),TUWeaponTest::Count(Restored->ExportItemLedger())+GroundRounds,61);
  TestFalse(TEXT("Dropped magazine never silently restored to actor"),Restored->ExportItemLedger().Magazines.ContainsByPredicate([Outgoing](const auto& M){return M.InstanceId==Outgoing;}));
  Restored->Destroy();
 }
 if(GEngine) GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponImportCompatibilityTest,"TheUnit.Combat.ImportAmmoCompatibility",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTUWeaponImportCompatibilityTest::RunTest(const FString& Parameters)
{
 UWorld* World=TUWeaponTest::World(); if(!World) return false;
 auto* W=World->SpawnActor<ATU_WeaponBase>(); const auto Original=TUWeaponTest::Kit(W); W->ImportItemLedger(Original);
 auto Bad=Original; Bad.Weapons[0].ChamberAmmoId=TEXT("Ammo_TU9_Ball"); TestFalse(TEXT("Foreign chamber rejected"),W->ImportItemLedger(Bad));
 Bad=Original; Bad.Magazines[0].Cartridges[0]=TEXT("Unknown_Ammo"); TestFalse(TEXT("Unknown round rejected"),W->ImportItemLedger(Bad));
 Bad=Original; Bad.Magazines[0].Cartridges[0]=TEXT("Ammo_TU9_Ball"); TestFalse(TEXT("Foreign round in magazine rejected"),W->ImportItemLedger(Bad));
 Bad=Original; Bad.Magazines[0].CompatibleAmmoId=TEXT("Ammo_TU9_Ball"); Bad.Magazines[0].Cartridges.Init(TEXT("Ammo_TU9_Ball"),30); TestFalse(TEXT("Foreign inserted magazine rejected"),W->ImportItemLedger(Bad));
 Bad=Original; Bad.Weapons[0].DefinitionId=TEXT("WPN_Foreign"); TestFalse(TEXT("Wrong weapon representation rejected"),W->ImportItemLedger(Bad));
 TestEqual(TEXT("Failed imports leave original revision"),W->ExportItemLedger().Revision,Original.Revision);
 TestEqual(TEXT("Failed imports leave 61 rounds"),TUWeaponTest::Count(W->ExportItemLedger()),61);
 auto Mixed=Original; Mixed.Magazines[0].Cartridges[0]=TEXT("Ammo_556_Training_Tracer"); Mixed.Magazines[0].Cartridges[1]=TEXT("Ammo_556_Training_Subsonic"); TestTrue(TEXT("Declared same family mixed variants accepted"),W->ImportItemLedger(Mixed));
 auto ForeignCarried=Original; ForeignCarried.Magazines[1].CompatibleAmmoId=TEXT("Ammo_TU9_Ball"); ForeignCarried.Magazines[1].Cartridges.Init(TEXT("Ammo_TU9_Ball"),30);
 TestTrue(TEXT("Foreign carried magazine remains valid loot"),W->ImportItemLedger(ForeignCarried));
 TestFalse(TEXT("Foreign carried magazine cannot be selected for reload"),W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Retain));
 if(GEngine) GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif