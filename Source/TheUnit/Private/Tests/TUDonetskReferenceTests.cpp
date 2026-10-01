#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TU_DonetskDistrictGenerator.h"
#include "TU_DonetskMissionGameMode.h"
#include "TU_GameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUDonetskGeneratorDefaultsTest,
    "TheUnit.Maps.Donetsk.ReferenceGenerator.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUDonetskGeneratorDefaultsTest::RunTest(const FString& Parameters)
{
    const ATU_DonetskDistrictGenerator* CDO = GetDefault<ATU_DonetskDistrictGenerator>();
    if (!TestNotNull(TEXT("Donetsk district generator CDO"), CDO))
    {
        return false;
    }

    TestTrue(TEXT("Donetsk reference generator is an actor"), CDO->IsA<AActor>());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUDonetskGameModeWiringTest,
    "TheUnit.Maps.Donetsk.GameMode.Wiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUDonetskGameModeWiringTest::RunTest(const FString& Parameters)
{
    const ATU_DonetskMissionGameMode* CDO = GetDefault<ATU_DonetskMissionGameMode>();
    if (!TestNotNull(TEXT("Donetsk GameMode CDO"), CDO))
    {
        return false;
    }

    TestTrue(TEXT("Donetsk mission GameMode inherits The Unit GameMode"), CDO->IsA<ATU_GameMode>());
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUDonetskHeroDressingTest,
    "TheUnit.Maps.Donetsk.HeroSlice.Dressing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUDonetskHeroDressingTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
        .AllowAudioPlayback(false)
        .CreatePhysicsScene(false)
        .CreateNavigation(false)
        .CreateAISystem(false)
        .ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(
        EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &InitValues);
    if (!TestNotNull(TEXT("Hero dressing test world"), World))
    {
        return false;
    }
    if (GEngine)
    {
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }

    ATU_DonetskDistrictGenerator* First = World->SpawnActor<ATU_DonetskDistrictGenerator>();
    ATU_DonetskDistrictGenerator* Second = World->SpawnActor<ATU_DonetskDistrictGenerator>();
    if (TestNotNull(TEXT("First hero district"), First)
        && TestNotNull(TEXT("Second hero district"), Second))
    {
        bool bBusShelter = false;
        bool bSedan = false;
        bool bRubble = false;
        TSet<FString> TreeTransforms;

        TInlineComponentArray<UStaticMeshComponent*> Meshes(First);
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            const FString Name = Mesh->GetName();
            bBusShelter |= Name.Contains(TEXT("TransitStop_Production"));
            bSedan |= Name.Contains(TEXT("CivilianSedan_Production"));
            bRubble |= Name.Contains(TEXT("Damage_RubblePile_Production"));

            UStaticMesh* StaticMesh = Mesh->GetStaticMesh();
            if (StaticMesh && !StaticMesh->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/Cube")))
            {
                TestEqual(TEXT("Hero production art never owns collision"),
                    Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }

            if (Name.Contains(TEXT("Tree")) && StaticMesh)
            {
                TreeTransforms.Add(
                    Mesh->GetRelativeScale3D().ToString()
                    + TEXT("|")
                    + Mesh->GetRelativeRotation().ToString());
            }
        }

        int32 HeroDecalCount = 0;
        bool bWetElement = false;
        TInlineComponentArray<UDecalComponent*> Decals(First);
        for (UDecalComponent* Decal : Decals)
        {
            if (Decal->GetName().Contains(TEXT("Hero")))
            {
                ++HeroDecalCount;
            }
            bWetElement |= Decal->GetName().Contains(TEXT("HeroWet"));
        }

        TestTrue(TEXT("Hero slice has production bus shelter"), bBusShelter);
        TestTrue(TEXT("Hero slice has production civilian sedan"), bSedan);
        TestTrue(TEXT("Hero slice has production rubble"), bRubble);
        TestTrue(TEXT("Hero slice has localized decals"), HeroDecalCount >= 6);
        TestTrue(TEXT("Hero slice has a wet/drain detail"), bWetElement);
        TestTrue(TEXT("Tree transforms are not repeated uniformly"), TreeTransforms.Num() > 1);
        TestEqual(TEXT("Hero dressing leaves collision deterministic"),
            First->GetGeneratedGeometrySignature(), Second->GetGeneratedGeometrySignature());
        TestEqual(TEXT("Hero dressing visual variation is deterministic"),
            First->GetGeneratedVisualSignature(), Second->GetGeneratedVisualSignature());
    }

    World->DestroyWorld(false);
    if (GEngine)
    {
        GEngine->DestroyWorldContext(World);
    }
    return true;
}

#endif
