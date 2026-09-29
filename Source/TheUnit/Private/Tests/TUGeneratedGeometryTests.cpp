#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "TU_DonetskDistrictGenerator.h"
#include "TU_DonetskArtema60Building.h"
#include "TU_KillhouseGenerator.h"
#include "TU_CommandCenterGenerator.h"
#include "TU_HideoutCommandCenterDecorator.h"
#include "TU_HideoutGameMode.h"
#include "TU_DonetskMissionGameMode.h"
#include "TU_TrainingMissionGameMode.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUGeneratedGeometryIdentityTest, "TheUnit.Presentation.GeneratedGeometryIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUGeneratedGeometryIdentityTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues InitValues = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &InitValues);
    if (!TestNotNull(TEXT("Geometry test world"), World)) return false;
    // ChildActorComponent cleanup uses the engine's world registry to destroy its actor.
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ATU_DonetskDistrictGenerator* First = World->SpawnActor<ATU_DonetskDistrictGenerator>();
    ATU_DonetskDistrictGenerator* Second = World->SpawnActor<ATU_DonetskDistrictGenerator>();
    if (TestNotNull(TEXT("First district"), First) && TestNotNull(TEXT("Second independent district"), Second))
    {
        TestTrue(TEXT("Generator has a real network actor identity"), First->GetIsReplicated());
        TestTrue(TEXT("Generator relevant across complete raid"), First->bAlwaysRelevant);
        TestTrue(TEXT("Generated collision exists"), First->GetGeneratedCollisionComponentCount() > 100);
        TestEqual(TEXT("Independent builds have identical named geometry"), First->GetGeneratedGeometrySignature(), Second->GetGeneratedGeometrySignature());
        TInlineComponentArray<UStaticMeshComponent*> Meshes(First);
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            TestTrue(TEXT("Collision component supports movement-base references"), Mesh->IsSupportedForNetworking());
            TestTrue(TEXT("Collision component name resolves relative to generator"), Mesh->IsNameStableForNetworking());
        }
    }
    ATU_DonetskArtema60Building* Building = World->SpawnActor<ATU_DonetskArtema60Building>();
    if (TestNotNull(TEXT("Reference building"), Building))
    {
        TestTrue(TEXT("Child building has network identity"), Building->GetIsReplicated());
        TestTrue(TEXT("Building collision generated"), Building->GetGeneratedCollisionComponentCount() > 10);
    }
    ATU_KillhouseGenerator* Killhouse = World->SpawnActor<ATU_KillhouseGenerator>();
    if (TestNotNull(TEXT("Training geometry"), Killhouse))
    {
        TestTrue(TEXT("Training generator has network identity"), Killhouse->GetIsReplicated());
        TestTrue(TEXT("Training collision generated"), Killhouse->GetGeneratedCollisionComponentCount() > 10);
    }
    ATU_CommandCenterGenerator* HQ = World->SpawnActor<ATU_CommandCenterGenerator>();
    if (TestNotNull(TEXT("Headquarters geometry"), HQ))
    {
        TestTrue(TEXT("Headquarters generator has network identity"), HQ->GetIsReplicated());
        TestTrue(TEXT("Headquarters collision generated"), HQ->GetGeneratedCollisionComponentCount() > 10);
    }
    ATU_HideoutCommandCenterDecorator* Decorator = World->SpawnActor<ATU_HideoutCommandCenterDecorator>();
    if (TestNotNull(TEXT("Progression decorator"), Decorator))
    {
        TestTrue(TEXT("Decorator has network identity"), Decorator->GetIsReplicated());
        TestTrue(TEXT("Progression-dependent collision generated"), Decorator->GetGeneratedCollisionComponentCount() > 0);
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHeadquartersWalkableBoundaryTest, "TheUnit.Presentation.HeadquartersWalkableBoundary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHeadquartersWalkableBoundaryTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues InitValues = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &InitValues);
    if (!TestNotNull(TEXT("HQ collision world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ATU_CommandCenterGenerator* HQ = World->SpawnActor<ATU_CommandCenterGenerator>();
    if (TestNotNull(TEXT("HQ collision geometry"), HQ))
    {
        FHitResult Floor;
        TestTrue(TEXT("Default player start has supporting floor"), World->LineTraceSingleByChannel(
            Floor, FVector(0,-3300,100), FVector(0,-3300,-100), ECC_Visibility));
        TestTrue(TEXT("Floor is at authored walkable height"), Floor.bBlockingHit && FMath::IsNearlyEqual(Floor.ImpactPoint.Z, 0.f, 1.f));
        for (const float StationY : {350.f, 600.f})
        {
            FHitResult Approach;
            TestTrue(TEXT("Raid station approach has real floor"), World->LineTraceSingleByChannel(
                Approach, FVector(450,StationY,150), FVector(450,StationY,-100), ECC_Visibility));
            TestTrue(TEXT("Station approach is at walkable height"), Approach.bBlockingHit && FMath::IsNearlyEqual(Approach.ImpactPoint.Z,0.f,1.f));
        }
        for (const float Direction : {-1.f, 1.f})
        {
            FHitResult Boundary;
            const bool Hit = World->SweepSingleByChannel(Boundary,
                FVector(0,Direction*3300.f,100), FVector(0,Direction*4200.f,100), FQuat::Identity,
                ECC_Pawn, FCollisionShape::MakeCapsule(34.f,88.f));
            TestTrue(TEXT("Walking capsule cannot leave either corridor end"), Hit && Boundary.bBlockingHit);
            TestTrue(TEXT("Boundary blocks before capsule leaves floor"), Hit && FMath::Abs(Boundary.Location.Y)<3800.f);
        }
    }
    // Exercise each actual fallback selection, rather than inspecting a copied
    // rotation constant. Negative-Y entry should look toward the playable center.
    for (UClass* ModeClass : {ATU_HideoutGameMode::StaticClass(), ATU_DonetskMissionGameMode::StaticClass(), ATU_TrainingMissionGameMode::StaticClass()})
    {
        AGameModeBase* Mode = World->SpawnActor<AGameModeBase>(ModeClass);
        AActor* Start = Mode ? Mode->ChoosePlayerStart(nullptr) : nullptr;
        if (TestNotNull(TEXT("Production fallback start"), Start))
        {
            const FVector TowardInterior = (-Start->GetActorLocation()).GetSafeNormal2D();
            TestTrue(TEXT("Fallback view faces playable interior from entry"),
                FVector::DotProduct(Start->GetActorForwardVector(), TowardInterior) > .99f);
            Start->Destroy();
        }
        if (Mode) Mode->Destroy();
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}
#endif
