#if WITH_DEV_AUTOMATION_TESTS
#include "TUWeaponPartsComponent.h"
#include "TU_TacticalRifle.h"
#include "TUWorldItem.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
namespace TUWeaponPartsTest
{
struct WorldScope {
 UWorld* World;
 WorldScope() { const auto V=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false); World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V); GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
 ~WorldScope() { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPartsCommittedStateTest,"TheUnit.Handling.PartsCommittedState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUPartsCommittedStateTest::RunTest(const FString&)
{
 TUWeaponPartsTest::WorldScope Scope;
 auto* W=Scope.World->SpawnActor<ATU_TacticalRifle>();
 auto Before=W->ExportItemLedger();
 auto F=UTUWeaponPartsComponent::Evaluate(Before,W->GetActionState(),0.f);
 TestEqual(TEXT("Seated visual uses actual inserted GUID"),F.InsertedId,Before.Weapons[0].InsertedMagazineId);
 TestFalse(TEXT("No detached magazine invented"),F.MovingId.IsValid());
 TestTrue(TEXT("Real retained reload accepted"),W->RequestReload(FGuid::NewGuid(),Before.Revision,ETUReloadPolicy::Retain));
 const FGuid Old=W->GetActionState().OutgoingMagazineId, New=W->GetActionState().ReplacementMagazineId;
 TestTrue(TEXT("Removal commits normally"),W->AdvanceActionClockForTesting());
 F=UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),.5f);
 TestFalse(TEXT("Empty well after removal"),F.InsertedId.IsValid());
 TestEqual(TEXT("Retained visual is same outgoing magazine"),F.MovingId,Old);
 const int32 Revision=W->ExportItemLedger().Revision;
 for(int i=0;i<100;++i) UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),i/100.f);
 TestEqual(TEXT("Cosmetic evaluation cannot advance inventory"),W->ExportItemLedger().Revision,Revision);
 TestTrue(TEXT("Acquire commits normally"),W->AdvanceActionClockForTesting());
 W->InterruptWeaponAction();
 F=UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),0.f);
 TestEqual(TEXT("Interrupted in-hand replacement remains the same item"),F.MovingId,New);
 TestFalse(TEXT("Interrupt does not seat a magazine"),F.InsertedId.IsValid());
 TestTrue(TEXT("Resume exact action"),W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Retain));
 TestTrue(TEXT("Insertion commits"),W->AdvanceActionClockForTesting());
 F=UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),.1f);
 TestEqual(TEXT("Seated replacement is correct GUID"),F.InsertedId,New);
 TestFalse(TEXT("Replacement never rendered twice"),F.MovingId.IsValid());
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPartsVisualAssetTest,"TheUnit.Handling.PartsVisualAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUPartsVisualAssetTest::RunTest(const FString&)
{
 TUWeaponPartsTest::WorldScope Scope;
 auto* W=Scope.World->SpawnActor<ATU_TacticalRifle>();
 auto* Source=W->GetWeaponBodyMesh();
 Source->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")));
 Source->SetRelativeRotation(FRotator(0,-90,0));
 W->ExportItemLedger(); auto* Parts=W->PartsPresentation.Get();
 Parts->UpdatePresentation(0.f);
 TestTrue(TEXT("Actual separated assets load"),Parts->IsSupported());
 TestFalse(TEXT("Original single-piece mesh not drawn twice"),Source->IsVisible());
 TestTrue(TEXT("Inserted model exists"),Parts->GetInsertedVisual() && Parts->GetInsertedVisual()->IsVisible());
 const FVector MagazineCenter=Parts->GetInsertedVisual()->GetComponentTransform().TransformPosition(Parts->GetInsertedVisual()->GetStaticMesh()->GetBounds().Origin);
 TestTrue(TEXT("Export/import basis returns magazine beneath receiver"),MagazineCenter.Equals(FVector(14.344508,.006448,-4.742494),.02f));
 const FVector Muzzle=W->GetWorldMuzzleLocation(); const auto Ledger=W->ExportItemLedger();
 FTUWeaponShotResult Shot; Shot.bFired=true; W->OnShotFired.Broadcast(Shot);
 Parts->UpdatePresentation(.001f);
 TestTrue(TEXT("Shot creates visible weapon kick"),Parts->GetWeaponKickCm()>.5f);
 TestTrue(TEXT("Cosmetic kick cannot move world muzzle"),W->GetWorldMuzzleLocation().Equals(Muzzle,.001f));
 TestEqual(TEXT("Cosmetic callback does not spend ammo"),W->ExportItemLedger().Revision,Ledger.Revision);
 Parts->UpdatePresentation(1.f); TestTrue(TEXT("Kick settles time-wise"),Parts->GetWeaponKickCm()<.001f);
 W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Drop);
 W->AdvanceActionClockForTesting(); // No pose tick: the commit must update visuals itself.
 TestFalse(TEXT("Emergency drop leaves no attached outgoing copy"),Parts->GetFrame().MovingId.IsValid());
 TestFalse(TEXT("Empty receiver after emergency drop"),Parts->GetInsertedVisual()->IsVisible());
 return true;
}
#endif
