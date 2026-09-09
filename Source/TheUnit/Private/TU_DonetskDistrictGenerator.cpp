#include "TU_DonetskDistrictGenerator.h"

#include "TU_DonetskArtema60Building.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATU_DonetskDistrictGenerator::ATU_DonetskDistrictGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Artema60Anchor = CreateDefaultSubobject<UChildActorComponent>(TEXT("Artema60Anchor"));
    Artema60Anchor->SetupAttachment(Root);
    Artema60Anchor->SetChildActorClass(ATU_DonetskArtema60Building::StaticClass());
    Artema60Anchor->SetRelativeLocation(FVector(-6200.0f, -5200.0f, 0.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeFinder.Object;
}

void ATU_DonetskDistrictGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ClearGenerated();
    GeneratedNameCounter = 0;
    RebuildDistrict();
}

void ATU_DonetskDistrictGenerator::RebuildDistrict()
{
    if (!CubeMesh)
    {
        return;
    }

    BuildRoadNetwork();
    BuildKhrushchyovkaCourtyard();
    BuildBrezhnevkaBlocks();
    BuildStalinistStreetWall();
    BuildRailStationReference();
    BuildIndustrialEdge();

    if (bGenerateTransitFurniture)
    {
        BuildStreetFurniture();
    }

    AddLabel(
        TEXT("REFERENCE ANCHOR // ARTEMA STREET 60 // DEDICATED PHOTO-MATCH ACTOR"),
        FVector(-6200.0f, -6500.0f, 1720.0f));
}

void ATU_DonetskDistrictGenerator::ClearGenerated()
{
    for (UActorComponent* Component : GeneratedComponents)
    {
        if (IsValid(Component))
        {
            Component->DestroyComponent();
        }
    }
    GeneratedComponents.Reset();
}

UStaticMeshComponent* ATU_DonetskDistrictGenerator::AddBox(
    const FVector& Location,
    const FVector& Extents,
    const FString& BaseName,
    const FRotator& Rotation)
{
    const FName Name(*FString::Printf(TEXT("%s_%04d"), *BaseName, GeneratedNameCounter++));
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, Name);
    Mesh->SetStaticMesh(CubeMesh);
    Mesh->SetRelativeLocation(Location);
    Mesh->SetRelativeRotation(Rotation);
    Mesh->SetRelativeScale3D(Extents / 50.0f);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Mesh->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Mesh->RegisterComponent();
    GeneratedComponents.Add(Mesh);
    return Mesh;
}

void ATU_DonetskDistrictGenerator::AddLabel(const FString& Text, const FVector& Location, const FRotator& Rotation)
{
    if (!bGenerateReferenceLabels)
    {
        return;
    }

    UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
    Label->SetText(FText::FromString(Text));
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetWorldSize(62.0f);
    Label->SetRelativeLocation(Location);
    Label->SetRelativeRotation(Rotation);
    Label->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Label->RegisterComponent();
    GeneratedComponents.Add(Label);
}

void ATU_DonetskDistrictGenerator::BuildRoadNetwork()
{
    // Original gameplay composition using dimensions/morphology visible in historical Donetsk street photography.
    // Main north/south boulevard is intentionally wide enough for trolley/tram infrastructure and divided traffic.
    const float GroundZ = -12.0f;
    AddBox(FVector::ZeroVector, FVector(DistrictWidthCm * 0.5f, DistrictLengthCm * 0.5f, 12.0f), TEXT("DistrictGround"));

    AddBox(FVector(0.0f, 0.0f, GroundZ + 10.0f), FVector(1650.0f, DistrictLengthCm * 0.5f, 10.0f), TEXT("MainBoulevard"));
    AddBox(FVector(0.0f, 0.0f, GroundZ + 22.0f), FVector(120.0f, DistrictLengthCm * 0.5f, 9.0f), TEXT("BoulevardMedian"));

    AddBox(FVector(0.0f, -9500.0f, GroundZ + 10.0f), FVector(DistrictWidthCm * 0.5f, 750.0f, 10.0f), TEXT("CrossStreetSouth"));
    AddBox(FVector(0.0f, 10200.0f, GroundZ + 10.0f), FVector(DistrictWidthCm * 0.5f, 720.0f, 10.0f), TEXT("CrossStreetNorth"));

    // Sidewalk bands create the broad, formal street edge characteristic of central Donetsk avenues.
    AddBox(FVector(-2050.0f, 0.0f, 5.0f), FVector(330.0f, DistrictLengthCm * 0.5f, 15.0f), TEXT("WestSidewalk"));
    AddBox(FVector(2050.0f, 0.0f, 5.0f), FVector(330.0f, DistrictLengthCm * 0.5f, 15.0f), TEXT("EastSidewalk"));

    AddLabel(TEXT("DONETSK REFERENCE BOULEVARD // PUBLIC CIVILIAN ARCHITECTURE STUDY"), FVector(0.0f, -28000.0f, 130.0f));
}

void ATU_DonetskDistrictGenerator::BuildSimpleFacadeBlock(
    const FVector& Origin,
    int32 Floors,
    int32 Bays,
    float BayWidthCm,
    float DepthCm,
    float FloorHeightCm,
    const FString& Prefix,
    bool bBalconies,
    bool bRaisedGroundFloor)
{
    const float Width = Bays * BayWidthCm;
    const float Height = Floors * FloorHeightCm;
    AddBox(Origin + FVector(0.0f, 0.0f, Height * 0.5f), FVector(Width * 0.5f, DepthCm * 0.5f, Height * 0.5f), Prefix + TEXT("_Mass"));

    const float FrontY = Origin.Y - DepthCm * 0.5f - 15.0f;
    for (int32 Floor = 0; Floor < Floors; ++Floor)
    {
        const float Z = Origin.Z + FloorHeightCm * (Floor + 0.5f);
        for (int32 Bay = 0; Bay < Bays; ++Bay)
        {
            const float X = Origin.X - Width * 0.5f + BayWidthCm * (Bay + 0.5f);
            const bool bEntrance = Floor == 0 && Bay == Bays / 2;
            const float WindowHeight = bRaisedGroundFloor && Floor == 0 ? FloorHeightCm * 0.52f : FloorHeightCm * 0.58f;
            AddBox(FVector(X, FrontY, Z), FVector(BayWidthCm * 0.26f, 18.0f, bEntrance ? FloorHeightCm * 0.38f : WindowHeight * 0.5f), Prefix + TEXT("_Opening"));

            if (bBalconies && Floor > 0 && (Bay % 2 == 1))
            {
                AddBox(FVector(X, FrontY - 70.0f, Z - FloorHeightCm * 0.18f), FVector(BayWidthCm * 0.34f, 65.0f, 12.0f), Prefix + TEXT("_BalconySlab"));
                AddBox(FVector(X, FrontY - 125.0f, Z - 5.0f), FVector(BayWidthCm * 0.34f, 8.0f, 55.0f), Prefix + TEXT("_BalconyRail"));
            }
        }
    }
}

void ATU_DonetskDistrictGenerator::BuildKhrushchyovkaCourtyard()
{
    // Typical Soviet 4-5 storey mass-housing morphology, used here because this housing type is pervasive across Ukrainian cities.
    const float FloorHeight = 280.0f;
    const FVector A(-10500.0f, 6500.0f, 0.0f);
    const FVector B(-10500.0f, 11800.0f, 0.0f);
    const FVector C(-15500.0f, 9100.0f, 0.0f);

    BuildSimpleFacadeBlock(A, 5, 16, 315.0f, 1150.0f, FloorHeight, TEXT("Khrush_A"), true, false);
    BuildSimpleFacadeBlock(B, 5, 14, 315.0f, 1150.0f, FloorHeight, TEXT("Khrush_B"), true, false);
    BuildSimpleFacadeBlock(C, 5, 12, 315.0f, 1150.0f, FloorHeight, TEXT("Khrush_C"), false, false);

    // Courtyard functions: narrow service road, playground pad, benches, drying/utility zone.
    AddBox(FVector(-12400.0f, 9200.0f, 4.0f), FVector(2600.0f, 1050.0f, 8.0f), TEXT("Khrush_CourtyardHardscape"));
    AddBox(FVector(-12300.0f, 9000.0f, 18.0f), FVector(650.0f, 420.0f, 18.0f), TEXT("Khrush_PlaygroundPad"));
    AddLabel(TEXT("5-STOREY KHRUSHCHEV-ERA COURTYARD TYPOLOGY"), FVector(-12400.0f, 7600.0f, 1650.0f));
}

void ATU_DonetskDistrictGenerator::BuildBrezhnevkaBlocks()
{
    const float FloorHeight = 285.0f;
    const FVector Origin(9800.0f, 6700.0f, 0.0f);

    BuildSimpleFacadeBlock(Origin, 9, 14, 340.0f, 1450.0f, FloorHeight, TEXT("Brezhnev_9F_A"), true, true);
    BuildSimpleFacadeBlock(Origin + FVector(5400.0f, 3300.0f, 0.0f), 9, 10, 340.0f, 1450.0f, FloorHeight, TEXT("Brezhnev_9F_B"), true, true);

    // Elevator/stair cores read as stronger vertical masses than Khrushchev-era blocks.
    AddBox(Origin + FVector(-2050.0f, -760.0f, 1280.0f), FVector(320.0f, 220.0f, 1280.0f), TEXT("Brezhnev_StairCore"));
    AddLabel(TEXT("9-STOREY BREZHNEV-ERA PANEL HOUSING TYPOLOGY"), Origin + FVector(0.0f, -1150.0f, 2850.0f));
}

void ATU_DonetskDistrictGenerator::BuildStalinistStreetWall()
{
    // Central-city street wall: taller floor-to-floor heights, formal frontage and raised ground floors.
    const FVector Origin(5900.0f, -2900.0f, 0.0f);
    BuildSimpleFacadeBlock(Origin, 5, 12, 390.0f, 1500.0f, 330.0f, TEXT("Stalinka_Block_A"), false, true);
    BuildSimpleFacadeBlock(Origin + FVector(5300.0f, 0.0f, 0.0f), 5, 10, 390.0f, 1500.0f, 330.0f, TEXT("Stalinka_Block_B"), true, true);
    AddLabel(TEXT("CENTRAL DONETSK STALIN-ERA STREET-WALL TYPOLOGY"), Origin + FVector(2400.0f, -1100.0f, 1850.0f));
}

void ATU_DonetskDistrictGenerator::BuildRailStationReference()
{
    // Public 2012 station imagery/reference: restored historic central volume linked to newer steel/glass transit additions.
    // This is a recognizable architectural study, not a survey-accurate station plan.
    const FVector Origin(10800.0f, 17600.0f, 0.0f);

    AddBox(Origin + FVector(0.0f, 0.0f, 520.0f), FVector(3000.0f, 1250.0f, 520.0f), TEXT("Station_HistoricCentralMass"));
    AddBox(Origin + FVector(-3550.0f, 150.0f, 390.0f), FVector(620.0f, 1050.0f, 390.0f), TEXT("Station_WestWing"));
    AddBox(Origin + FVector(3550.0f, 150.0f, 390.0f), FVector(620.0f, 1050.0f, 390.0f), TEXT("Station_EastWing"));

    // Modernized transit/concourse volumes: lower steel/glass forms bridging old and new.
    AddBox(Origin + FVector(0.0f, 2200.0f, 330.0f), FVector(4300.0f, 820.0f, 330.0f), TEXT("Station_2012Concourse"));
    AddBox(Origin + FVector(0.0f, 3200.0f, 520.0f), FVector(1200.0f, 180.0f, 520.0f), TEXT("Station_GlassAtrium_Blockout"));

    for (int32 Bay = -6; Bay <= 6; ++Bay)
    {
        AddBox(Origin + FVector(Bay * 420.0f, -1270.0f, 480.0f), FVector(110.0f, 22.0f, 180.0f), TEXT("Station_FacadeBay"));
    }

    AddLabel(TEXT("REFERENCE ANCHOR // DONETSK RAILWAY STATION // 1951 CORE + 2012 EXPANSION"), Origin + FVector(0.0f, -1550.0f, 1250.0f));
}

void ATU_DonetskDistrictGenerator::BuildIndustrialEdge()
{
    const FVector Origin(-15500.0f, -15000.0f, 0.0f);
    AddBox(Origin + FVector(0.0f, 0.0f, 450.0f), FVector(4200.0f, 1800.0f, 450.0f), TEXT("Industrial_Warehouse"));
    AddBox(Origin + FVector(4800.0f, 400.0f, 320.0f), FVector(1700.0f, 1200.0f, 320.0f), TEXT("Industrial_Workshop"));
    AddBox(Origin + FVector(2000.0f, -2500.0f, 8.0f), FVector(6000.0f, 1200.0f, 8.0f), TEXT("Industrial_ServiceYard"));

    for (int32 Index = 0; Index < 8; ++Index)
    {
        const float X = Origin.X - 4500.0f + Index * 1350.0f;
        AddBox(FVector(X, Origin.Y - 3650.0f, 120.0f), FVector(18.0f, 18.0f, 120.0f), TEXT("Industrial_FencePost"));
    }

    AddLabel(TEXT("DONBAS INDUSTRIAL EDGE // WAREHOUSE + SERVICE YARD"), Origin + FVector(1000.0f, -2500.0f, 1050.0f));
}

void ATU_DonetskDistrictGenerator::BuildStreetFurniture()
{
    // Trolley/tram visual language is supported by historical Artema Street photos and Donetsk transport maps.
    for (int32 Index = -7; Index <= 7; ++Index)
    {
        const float Y = Index * 3600.0f;
        AddBox(FVector(-1480.0f, Y, 420.0f), FVector(18.0f, 18.0f, 420.0f), TEXT("TransitPole_W"));
        AddBox(FVector(1480.0f, Y + 1800.0f, 420.0f), FVector(18.0f, 18.0f, 420.0f), TEXT("TransitPole_E"));
    }

    // Bus/tram stop shelter blockout.
    AddBox(FVector(2350.0f, -7200.0f, 115.0f), FVector(360.0f, 90.0f, 115.0f), TEXT("TransitStop_Back"));
    AddBox(FVector(2350.0f, -7200.0f, 240.0f), FVector(390.0f, 130.0f, 14.0f), TEXT("TransitStop_Roof"));

    // Kiosks and utility cabinets are part of the everyday street texture rather than combat-specific set dressing.
    AddBox(FVector(-2800.0f, 3400.0f, 120.0f), FVector(180.0f, 140.0f, 120.0f), TEXT("StreetKiosk_A"));
    AddBox(FVector(-2800.0f, 3850.0f, 120.0f), FVector(180.0f, 140.0f, 120.0f), TEXT("StreetKiosk_B"));
    AddBox(FVector(2750.0f, 11800.0f, 75.0f), FVector(90.0f, 70.0f, 75.0f), TEXT("UtilityCabinet"));
}