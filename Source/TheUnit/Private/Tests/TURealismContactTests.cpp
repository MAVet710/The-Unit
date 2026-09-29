#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TUWeaponContactProfile.h"
#include "TUHandlingCameraComponent.h"
#include "Camera/CameraTypes.h"
#include "TUWeaponClearance.h"
#include "TUWeaponPartsComponent.h"
#include "TU_TacticalRifle.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
namespace TUContactTest
{
struct FWorldScope {
    UWorld* World;
    FWorldScope() {
        const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    ~FWorldScope(){ World->DestroyWorld(false);GEngine->DestroyWorldContext(World); }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUContactCycleTest,"TheUnit.Handling.ContactCycleDistinction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUContactCycleTest::RunTest(const FString&)
{
    TUContactTest::FWorldScope Scope;
    for(bool Empty:{false,true}) {
        auto* W=Scope.World->SpawnActor<ATU_TacticalRifle>();
        auto Ledger=W->ExportItemLedger();
        if(Empty) { Ledger.Weapons[0].ChamberAmmoId=NAME_None;Ledger.Weapons[0].bActionOpen=true;TestTrue(TEXT("Empty fixture imported"),W->ImportItemLedger(Ledger)); }
        TestTrue(TEXT("Normal authoritative reload"),W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Retain));
        TestEqual(TEXT("Empty and tactical actions are distinct"),W->GetActionState().bRequiresChamberCycle,Empty);
        auto Frame=UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),1.f);
        TestEqual(TEXT("Grip closes before removal"),Frame.MagazineGripAlpha,1.f);
        W->AdvanceActionClockForTesting();W->AdvanceActionClockForTesting();W->AdvanceActionClockForTesting();
        Frame=UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),1.f);
        TestEqual(TEXT("Only empty reload touches action control"),Frame.ControlGripAlpha,Empty?1.f:0.f);
        TestEqual(TEXT("Only empty reload presses action control"),Frame.ControlPress,Empty?1.f:0.f);
        const int32 Revision=W->ExportItemLedger().Revision;
        for(int32 I=0;I<60;++I) UTUWeaponPartsComponent::Evaluate(W->ExportItemLedger(),W->GetActionState(),I/60.f);
        TestEqual(TEXT("Contact animation never commits inventory"),W->ExportItemLedger().Revision,Revision);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUContactClearanceTest,"TheUnit.Handling.PhysicalCornerClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUContactClearanceTest::RunTest(const FString&)
{
    TUContactTest::FWorldScope Scope;
    auto* Wall=Scope.World->SpawnActor<AActor>();
    auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(5,150,150));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();
    Wall->SetActorLocation(FVector(100,0,100));
    const FBox Bounds(FVector(-27,-4,-12),FVector(51,4,18));
    const FTransform Desired(FVector(60,0,100)),Eye(FVector(18,0,100)),Previous(FVector(0,0,100));
    TestFalse(TEXT("Actual world wall blocks extended mesh volume"),TUWeaponClearance::IsClear(Scope.World,nullptr,nullptr,Desired,Bounds));
    const auto Result=TUWeaponClearance::Resolve(Scope.World,nullptr,nullptr,Desired,Eye,Bounds,Previous);
    TestTrue(TEXT("Obstruction remains explicit for firing gate"),Result.bDesiredBlocked);
    TestTrue(TEXT("Physical ready pose resolved"),Result.bResolved);
    TestTrue(TEXT("Resolved world representation does not penetrate wall"),TUWeaponClearance::IsClear(Scope.World,nullptr,nullptr,Result.Pose,Bounds));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUContactFingerTest,"TheUnit.Handling.IndependentFingerGrips",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUContactFingerTest::RunTest(const FString&)
{
    const FTransform Rest(FRotator(15,30,10),FVector(2,0,0));
    TestTrue(TEXT("Firing hand does not inherit support-hand gestures"),TUWeaponContact::FingerPose(TEXT("index_02_r"),Rest,1.f,1.f).Equals(Rest));
    TestEqual(TEXT("Finger segments never change length"),TUWeaponContact::FingerPose(TEXT("middle_02_l"),Rest,1.f,0.f).GetTranslation().Size(),Rest.GetTranslation().Size());
    TestFalse(TEXT("Magazine and control finger poses differ"),TUWeaponContact::FingerPose(TEXT("index_02_l"),Rest,1.f,0.f).Equals(TUWeaponContact::FingerPose(TEXT("index_02_l"),Rest,0.f,1.f)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUContactViewTest,"TheUnit.Handling.CloseViewPreservesWorldPose",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUContactViewTest::RunTest(const FString&)
{
    TUContactTest::FWorldScope Scope;
    auto* Actor=Scope.World->SpawnActor<AActor>();
    auto* Camera=NewObject<UTUHandlingCameraComponent>(Actor);Actor->SetRootComponent(Camera);Camera->RegisterComponent();
    Actor->SetActorLocation(FVector(10,20,30));Camera->SetFieldOfView(90.f);
    const FTransform Before=Actor->GetActorTransform();FMinimalViewInfo View;Camera->GetCameraView(0.f,View);
    TestEqual(TEXT("Nearby stock surfaces are not cut at ten centimeters"),View.PerspectiveNearClipPlane,2.f);
    TestTrue(TEXT("Near plane never moves physical actor"),Actor->GetActorTransform().Equals(Before));
    TestTrue(TEXT("View location still equals physical camera"),View.Location.Equals(Camera->GetComponentLocation()));
    TestEqual(TEXT("Near plane never silently changes FOV"),View.FOV,90.f);
    return true;
}
#endif
