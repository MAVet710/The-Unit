#include "TU_DonetskDistrictGenerator.h"

#include "TU_DonetskArtema60Building.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Misc/Crc.h"

ATU_DonetskDistrictGenerator::ATU_DonetskDistrictGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Static);
    SetRootComponent(Root);

    Artema60Anchor = CreateDefaultSubobject<UChildActorComponent>(TEXT("Artema60Anchor"));
    Artema60Anchor->SetupAttachment(Root);
    Artema60Anchor->SetIsReplicated(true);
    Artema60Anchor->SetChildActorClass(ATU_DonetskArtema60Building::StaticClass());
    Artema60Anchor->SetRelativeLocation(FVector(-6200.0f, -5200.0f, 0.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Khrush16Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Khrush_5F_16.SM_Donetsk_Khrush_5F_16"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Khrush14Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Khrush_5F_14.SM_Donetsk_Khrush_5F_14"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Khrush12Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Khrush_5F_12.SM_Donetsk_Khrush_5F_12"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Brezhnev14Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Brezhnev_9F_14.SM_Donetsk_Brezhnev_9F_14"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Brezhnev10Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Brezhnev_9F_10.SM_Donetsk_Brezhnev_9F_10"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Stalinka12Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Stalinka_5F_12.SM_Donetsk_Stalinka_5F_12"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Stalinka10Finder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_Stalinka_5F_10.SM_Donetsk_Stalinka_5F_10"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeAFinder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_StreetTree_A.SM_Donetsk_StreetTree_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeBFinder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_StreetTree_B.SM_Donetsk_StreetTree_B"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BusShelterFinder(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_BusShelter.SM_Donetsk_Prop_BusShelter"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SedanFinder(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Sedan.SM_Donetsk_Prop_Sedan"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RubblePileFinder(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_RubblePile.SM_Donetsk_Prop_RubblePile"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CivicCoreFinder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_CivicCore.SM_Donetsk_CivicCore"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CivicWingFinder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_CivicWing.SM_Donetsk_CivicWing"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RailStationFinder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_RailStation_Reference.SM_Donetsk_RailStation_Reference"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> IndustrialFinder(TEXT("/Game/TheUnit/Donetsk/Production/SM_Donetsk_IndustrialEdge.SM_Donetsk_IndustrialEdge"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> AsphaltFinder(TEXT("/Game/TheUnit/Donetsk/Materials/MI_Donetsk_Asphalt.MI_Donetsk_Asphalt"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PavingFinder(TEXT("/Game/TheUnit/Donetsk/Materials/MI_Donetsk_Paving.MI_Donetsk_Paving"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrassFinder(TEXT("/Game/TheUnit/Donetsk/Production/UrbanGrass.UrbanGrass"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> SoilFinder(TEXT("/Game/TheUnit/Donetsk/Materials/MI_Donetsk_Soil.MI_Donetsk_Soil"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> RustFinder(TEXT("/Game/TheUnit/Donetsk/Materials/MI_Donetsk_RustedSteel.MI_Donetsk_RustedSteel"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ConcreteFinder(TEXT("/Game/TheUnit/Donetsk/Materials/MI_Donetsk_AgedConcrete.MI_Donetsk_AgedConcrete"));
    CubeMesh = CubeFinder.Object;
    Khrush16Mesh = Khrush16Finder.Object; Khrush14Mesh = Khrush14Finder.Object; Khrush12Mesh = Khrush12Finder.Object;
    Brezhnev14Mesh = Brezhnev14Finder.Object; Brezhnev10Mesh = Brezhnev10Finder.Object;
    Stalinka12Mesh = Stalinka12Finder.Object; Stalinka10Mesh = Stalinka10Finder.Object;
    StreetTreeAMesh = TreeAFinder.Object; StreetTreeBMesh = TreeBFinder.Object;
    BusShelterMesh = BusShelterFinder.Object; SedanMesh = SedanFinder.Object; RubblePileMesh = RubblePileFinder.Object;
    CivicCoreMesh = CivicCoreFinder.Object; CivicWingMesh = CivicWingFinder.Object;
    RailStationMesh = RailStationFinder.Object; IndustrialEdgeMesh = IndustrialFinder.Object;
    AsphaltMaterial = AsphaltFinder.Object; PavingMaterial = PavingFinder.Object;
    GrassMaterial = GrassFinder.Object; SoilMaterial = SoilFinder.Object;
    RustMaterial = RustFinder.Object; ConcreteMaterial = ConcreteFinder.Object;
}

void ATU_DonetskDistrictGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (GetNetMode() == NM_Client && !bInitialLayoutReceived) return;
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
    BuildCentralSquareReference();
    BuildKhrushchyovkaCourtyard();
    BuildBrezhnevkaBlocks();
    BuildStalinistStreetWall();
    BuildRailStationReference();
    BuildIndustrialEdge();

    if (bGenerateTransitFurniture)
    {
        BuildStreetFurniture();
    }
    if (bGenerateUrbanVegetation)
    {
        BuildUrbanVegetation();
    }
    if (bGenerateMissionDamage)
    {
        BuildMissionDamageLayer();
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
    Mesh->SetNetAddressable();
    Mesh->SetStaticMesh(CubeMesh);
    Mesh->SetRelativeLocation(Location);
    Mesh->SetRelativeRotation(Rotation);
    Mesh->SetRelativeScale3D(Extents / 50.0f);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

    UMaterialInterface* Surface = nullptr;
    if (BaseName.Contains(TEXT("Boulevard")) || BaseName.Contains(TEXT("Street")) ||
        BaseName.Contains(TEXT("Road")) || BaseName.Contains(TEXT("Asphalt")))
        Surface = AsphaltMaterial;
    else if (BaseName.Contains(TEXT("Sidewalk")) || BaseName.Contains(TEXT("Hardscape")) ||
             BaseName.Contains(TEXT("Apron")) || BaseName.Contains(TEXT("Steps")) ||
             BaseName.Contains(TEXT("ServiceYard")) || BaseName.Contains(TEXT("Playground")))
        Surface = PavingMaterial;
    else if (BaseName.Contains(TEXT("Parterre")) || BaseName.Contains(TEXT("Median")))
        Surface = GrassMaterial;
    else if (BaseName.Contains(TEXT("DistrictGround")))
        Surface = SoilMaterial;
    else if (BaseName.Contains(TEXT("Pole")) || BaseName.Contains(TEXT("Wire")) ||
             BaseName.Contains(TEXT("Rail")) || BaseName.Contains(TEXT("Fence")) ||
             BaseName.Contains(TEXT("Bollard")) || BaseName.Contains(TEXT("Cabinet")) ||
             BaseName.Contains(TEXT("Damage_Rust")))
        Surface = RustMaterial;
    else if (BaseName.Contains(TEXT("Damage_Rubble")) || BaseName.Contains(TEXT("Civic")) ||
             BaseName.Contains(TEXT("Industrial")) || BaseName.Contains(TEXT("Station")))
        Surface = ConcreteMaterial;
    if (Surface) Mesh->SetMaterial(0, Surface);

    Mesh->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Mesh->RegisterComponent();
    GeneratedComponents.Add(Mesh);
    return Mesh;
}

UStaticMeshComponent* ATU_DonetskDistrictGenerator::AddHiddenBox(
    const FVector& Location,
    const FVector& Extents,
    const FString& BaseName,
    const FRotator& Rotation)
{
    UStaticMeshComponent* Mesh = AddBox(Location, Extents, BaseName, Rotation);
    if (Mesh)
    {
        // Visibility is disabled while collision stays active. This is intentionally
        // the same proven API path used elsewhere in the project.
        Mesh->SetVisibility(false, true);
    }
    return Mesh;
}

UStaticMeshComponent* ATU_DonetskDistrictGenerator::AddProductionVisual(
    UStaticMesh* Asset, const FVector& Location, const FString& BaseName, const FRotator& Rotation)
{
    if (!Asset) return nullptr;
    const FName Name(*FString::Printf(TEXT("%s_%04d"), *BaseName, GeneratedNameCounter++));
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, Name);
    Mesh->SetNetAddressable();
    Mesh->SetStaticMesh(Asset);
    Mesh->SetRelativeLocation(Location);
    Mesh->SetRelativeRotation(Rotation);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCastShadow(true);
    Mesh->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Mesh->RegisterComponent();
    GeneratedComponents.Add(Mesh);
    return Mesh;
}

void ATU_DonetskDistrictGenerator::AddLabel(const FString& Text, const FVector& Location, const FRotator& Rotation)
{
#if WITH_EDITOR
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
#else
    // Reference labels are editor calibration aids only. Explicitly consume
    // parameters so packaged builds compile cleanly with warnings-as-errors.
    (void)Text;
    (void)Location;
    (void)Rotation;
#endif
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

void ATU_DonetskDistrictGenerator::BuildCentralSquareReference()
{
    // Documented planning anchor: the central square was laid out as an elongated
    // approximately 140 m x 480 m rectangle east of Artema Street between the
    // Komsomolskyi and Gurova cross axes. This establishes real-world scale and
    // street hierarchy; individual civic-building dimensions remain photo-match work.
    const FVector SquareCenter(10000.0f, 0.0f, 0.0f);
    AddBox(SquareCenter + FVector(0.0f, 0.0f, 3.0f),
        FVector(7000.0f, 24000.0f, 8.0f), TEXT("CentralSquare_Hardscape"));

    // Artema-side pedestrian apron and eastern service street frame the square.
    AddBox(FVector(2750.0f, 0.0f, 8.0f), FVector(350.0f, 24000.0f, 12.0f), TEXT("CentralSquare_WestApron"));
    AddBox(FVector(18150.0f, 0.0f, 0.0f), FVector(650.0f, 24000.0f, 10.0f), TEXT("CentralSquare_EastStreet"));

    // Parterre / planted end zones retain the large civic scale without filling the
    // entire plaza with collision-heavy detail.
    AddBox(FVector(10000.0f, -21500.0f, 14.0f), FVector(6100.0f, 1700.0f, 14.0f), TEXT("CentralSquare_SouthParterre"));
    AddBox(FVector(10000.0f, 21500.0f, 14.0f), FVector(6100.0f, 1700.0f, 14.0f), TEXT("CentralSquare_NorthParterre"));

    const int32 CivicCollisionStart = GeneratedComponents.Num();
    // Photo-match working civic edge. Keep these clean; mission damage is authored
    // as a separate layer so reference proportions do not get baked into rubble.
    AddBox(FVector(22100.0f, 0.0f, 900.0f), FVector(2850.0f, 4100.0f, 900.0f),
        TEXT("CentralSquare_CivicCore"), FRotator(0.0f, 90.0f, 0.0f));
    AddBox(FVector(21350.0f, -11000.0f, 700.0f), FVector(2200.0f, 3300.0f, 700.0f),
        TEXT("CentralSquare_CivicSouth"), FRotator(0.0f, 90.0f, 0.0f));
    AddBox(FVector(21350.0f, 11000.0f, 700.0f), FVector(2200.0f, 3300.0f, 700.0f),
        TEXT("CentralSquare_CivicNorth"), FRotator(0.0f, 90.0f, 0.0f));

    // Formal stair/column rhythm on the central civic frontage.
    AddBox(FVector(18680.0f, 0.0f, 45.0f), FVector(520.0f, 2500.0f, 45.0f), TEXT("CentralSquare_CivicSteps"));
    for (int32 Column = -5; Column <= 5; ++Column)
    {
        AddBox(FVector(19000.0f, Column * 390.0f, 480.0f),
            FVector(22.0f, 22.0f, 480.0f), TEXT("CentralSquare_Column"));
    }

    if (CivicCoreMesh && CivicWingMesh)
    {
        for (int32 Index = CivicCollisionStart; Index < GeneratedComponents.Num(); ++Index)
            if (UStaticMeshComponent* CollisionPiece = Cast<UStaticMeshComponent>(GeneratedComponents[Index]))
                CollisionPiece->SetVisibility(false, true);
        AddProductionVisual(CivicCoreMesh, FVector(22100.0f, 0.0f, 0.0f),
            TEXT("CentralSquare_CivicCore_Production"), FRotator(0.0f, 90.0f, 0.0f));
        AddProductionVisual(CivicWingMesh, FVector(21350.0f, -11000.0f, 0.0f),
            TEXT("CentralSquare_CivicSouth_Production"), FRotator(0.0f, 90.0f, 0.0f));
        AddProductionVisual(CivicWingMesh, FVector(21350.0f, 11000.0f, 0.0f),
            TEXT("CentralSquare_CivicNorth_Production"), FRotator(0.0f, 90.0f, 0.0f));
    }

    // Human-scale benches and bollards prevent the plaza from reading as an empty
    // game arena while preserving long civic sightlines.
    for (int32 Row = -3; Row <= 3; ++Row)
    {
        const float Y = Row * 5200.0f;
        AddHiddenBox(FVector(4300.0f, Y, 45.0f), FVector(145.0f, 38.0f, 45.0f), TEXT("CentralSquare_BenchW"));
        AddHiddenBox(FVector(15500.0f, Y + 1800.0f, 45.0f), FVector(145.0f, 38.0f, 45.0f), TEXT("CentralSquare_BenchE"));
    }

    AddLabel(TEXT("CENTRAL CIVIC SQUARE // DOCUMENTED 140m x 480m PLANNING SCALE"),
        FVector(10000.0f, 0.0f, 2050.0f), FRotator(0.0f, 180.0f, 0.0f));
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
    const int32 CollisionStartIndex = GeneratedComponents.Num();
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

    UStaticMesh* Production = nullptr;
    if (Prefix == TEXT("Khrush_A")) Production = Khrush16Mesh;
    else if (Prefix == TEXT("Khrush_B")) Production = Khrush14Mesh;
    else if (Prefix == TEXT("Khrush_C")) Production = Khrush12Mesh;
    else if (Prefix == TEXT("Brezhnev_9F_A")) Production = Brezhnev14Mesh;
    else if (Prefix == TEXT("Brezhnev_9F_B")) Production = Brezhnev10Mesh;
    else if (Prefix == TEXT("Stalinka_Block_A")) Production = Stalinka12Mesh;
    else if (Prefix == TEXT("Stalinka_Block_B")) Production = Stalinka10Mesh;

    if (Production)
    {
        // Production mesh owns rendering; generated pieces remain invisible, stable
        // collision so network identities and traversal do not depend on imported art.
        for (int32 Index = CollisionStartIndex; Index < GeneratedComponents.Num(); ++Index)
            if (UStaticMeshComponent* CollisionPiece = Cast<UStaticMeshComponent>(GeneratedComponents[Index]))
                CollisionPiece->SetVisibility(false, true);
        AddProductionVisual(Production, Origin, Prefix + TEXT("_ProductionVisual"));
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
    const FVector Origin(19700.0f, 7600.0f, 0.0f);

    BuildSimpleFacadeBlock(Origin, 9, 14, 340.0f, 1450.0f, FloorHeight, TEXT("Brezhnev_9F_A"), true, true);
    BuildSimpleFacadeBlock(Origin + FVector(1500.0f, 5200.0f, 0.0f), 9, 10, 340.0f, 1450.0f, FloorHeight, TEXT("Brezhnev_9F_B"), true, true);

    // Elevator/stair cores read as stronger vertical masses than Khrushchev-era blocks.
    AddHiddenBox(Origin + FVector(-2050.0f, -760.0f, 1280.0f), FVector(320.0f, 220.0f, 1280.0f), TEXT("Brezhnev_StairCore"));
    AddLabel(TEXT("9-STOREY BREZHNEV-ERA PANEL HOUSING TYPOLOGY"), Origin + FVector(0.0f, -1150.0f, 2850.0f));
}

void ATU_DonetskDistrictGenerator::BuildStalinistStreetWall()
{
    // Central-city street wall: taller floor-to-floor heights, formal frontage and raised ground floors.
    const FVector Origin(7600.0f, 26300.0f, 0.0f);
    BuildSimpleFacadeBlock(Origin, 5, 12, 390.0f, 1500.0f, 330.0f, TEXT("Stalinka_Block_A"), false, true);
    BuildSimpleFacadeBlock(Origin + FVector(5300.0f, 0.0f, 0.0f), 5, 10, 390.0f, 1500.0f, 330.0f, TEXT("Stalinka_Block_B"), true, true);
    AddLabel(TEXT("CENTRAL DONETSK STALIN-ERA STREET-WALL TYPOLOGY"), Origin + FVector(2400.0f, -1100.0f, 1850.0f));
}

void ATU_DonetskDistrictGenerator::BuildRailStationReference()
{
    // Public 2012 station imagery/reference: restored historic central volume linked to newer steel/glass transit additions.
    // This is a recognizable architectural study, not a survey-accurate station plan.
    const FVector Origin(-17000.0f, 23800.0f, 0.0f);
    const int32 StationCollisionStart = GeneratedComponents.Num();

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
    if (RailStationMesh)
    {
        for (int32 Index = StationCollisionStart; Index < GeneratedComponents.Num(); ++Index)
            if (UStaticMeshComponent* CollisionPiece = Cast<UStaticMeshComponent>(GeneratedComponents[Index]))
                CollisionPiece->SetVisibility(false, true);
        AddProductionVisual(RailStationMesh, Origin, TEXT("Station_ProductionVisual"));
    }

    AddLabel(TEXT("REFERENCE ANCHOR // DONETSK RAILWAY STATION // 1951 CORE + 2012 EXPANSION"), Origin + FVector(0.0f, -1550.0f, 1250.0f));
}

void ATU_DonetskDistrictGenerator::BuildIndustrialEdge()
{
    const FVector Origin(-15500.0f, -15000.0f, 0.0f);
    UStaticMeshComponent* WarehouseCollision = AddBox(
        Origin + FVector(0.0f, 0.0f, 450.0f), FVector(4200.0f, 1800.0f, 450.0f), TEXT("Industrial_Warehouse"));
    UStaticMeshComponent* WorkshopCollision = AddBox(
        Origin + FVector(4800.0f, 400.0f, 320.0f), FVector(1700.0f, 1200.0f, 320.0f), TEXT("Industrial_Workshop"));
    AddBox(Origin + FVector(2000.0f, -2500.0f, 8.0f), FVector(6000.0f, 1200.0f, 8.0f), TEXT("Industrial_ServiceYard"));

    for (int32 Index = 0; Index < 8; ++Index)
    {
        const float X = Origin.X - 4500.0f + Index * 1350.0f;
        AddHiddenBox(FVector(X, Origin.Y - 3650.0f, 120.0f), FVector(18.0f, 18.0f, 120.0f), TEXT("Industrial_FencePost"));
    }
    if (IndustrialEdgeMesh)
    {
        if (WarehouseCollision) WarehouseCollision->SetVisibility(false, true);
        if (WorkshopCollision) WorkshopCollision->SetVisibility(false, true);
        AddProductionVisual(IndustrialEdgeMesh, Origin, TEXT("Industrial_ProductionVisual"));
    }

    AddLabel(TEXT("DONBAS INDUSTRIAL EDGE // WAREHOUSE + SERVICE YARD"), Origin + FVector(1000.0f, -2500.0f, 1050.0f));
}

void ATU_DonetskDistrictGenerator::BuildStreetFurniture()
{
    // Trolley/tram visual language is supported by historical Artema Street photos and Donetsk transport maps.
    for (int32 Index = -7; Index <= 7; ++Index)
    {
        const float Y = Index * 3600.0f;
        AddHiddenBox(FVector(-1480.0f, Y, 420.0f), FVector(18.0f, 18.0f, 420.0f), TEXT("TransitPole_W"));
        AddHiddenBox(FVector(1480.0f, Y + 1800.0f, 420.0f), FVector(18.0f, 18.0f, 420.0f), TEXT("TransitPole_E"));
    }

    // Trolley-contact wire pair and cross spans. Thin collision boxes are deliberate
    // at this stage: they establish authentic vertical scale and wire rhythm while
    // remaining deterministic across network peers.
    for (float WireX : {-720.0f, 720.0f})
        AddHiddenBox(FVector(WireX, 0.0f, 610.0f), FVector(6.0f, DistrictLengthCm * 0.46f, 4.0f), TEXT("TransitWire_Longitudinal"));
    for (int32 Index = -7; Index <= 7; ++Index)
        AddHiddenBox(FVector(0.0f, Index * 3600.0f, 630.0f), FVector(1480.0f, 5.0f, 4.0f), TEXT("TransitWire_CrossSpan"));

    // Bus/tram stop collision remains hidden and deterministic. Production art owns rendering.
    AddHiddenBox(FVector(2350.0f, -7200.0f, 115.0f), FVector(360.0f, 90.0f, 115.0f), TEXT("TransitStop_Back"));
    AddHiddenBox(FVector(2350.0f, -7200.0f, 240.0f), FVector(390.0f, 130.0f, 14.0f), TEXT("TransitStop_Roof"));
    AddProductionVisual(BusShelterMesh, FVector(2350.0f, -7200.0f, 0.0f),
        TEXT("TransitStop_Production"));

    // A civilian sedan adds real-world street scale. Its simple collision remains independent
    // from the render mesh so source-art revisions cannot change authoritative traversal.
    AddHiddenBox(FVector(3550.0f, -4400.0f, 75.0f), FVector(215.0f, 90.0f, 75.0f),
        TEXT("CivilianSedan_Collision"), FRotator(0.0f, 90.0f, 0.0f));
    AddProductionVisual(SedanMesh, FVector(3550.0f, -4400.0f, 0.0f),
        TEXT("CivilianSedan_Production"), FRotator(0.0f, 90.0f, 0.0f));

    // Kiosks and utility cabinets are part of the everyday street texture rather than combat-specific set dressing.
    AddHiddenBox(FVector(-2800.0f, 3400.0f, 120.0f), FVector(180.0f, 140.0f, 120.0f), TEXT("StreetKiosk_A"));
    AddHiddenBox(FVector(-2800.0f, 3850.0f, 120.0f), FVector(180.0f, 140.0f, 120.0f), TEXT("StreetKiosk_B"));
    AddHiddenBox(FVector(2750.0f, 11800.0f, 75.0f), FVector(90.0f, 70.0f, 75.0f), TEXT("UtilityCabinet"));
}

void ATU_DonetskDistrictGenerator::BuildUrbanVegetation()
{
    // Mature deciduous street-tree rhythm breaks the boulevard into human-scale
    // sightline segments. Asset choice/spacing are original composition; no claim
    // is made that individual trees correspond to a current real-world specimen.
    int32 TreeIndex = 0;
    for (int32 Y = -25200; Y <= 25200; Y += 4200)
    {
        for (float Side : {-1.0f, 1.0f})
        {
            const float X = Side * 2580.0f;
            UStaticMesh* Tree = ((TreeIndex++ % 3) == 0) ? StreetTreeBMesh : StreetTreeAMesh;
            AddProductionVisual(Tree, FVector(X, static_cast<float>(Y), 0.0f), TEXT("BoulevardTree"));
            if (UStaticMeshComponent* Trunk = AddBox(
                FVector(X, static_cast<float>(Y), 420.0f),
                FVector(46.0f, 46.0f, 420.0f), TEXT("Vegetation_TrunkCollision")))
                Trunk->SetVisibility(false, true);
        }
    }

    // Central-square perimeter trees maintain the documented open civic core while
    // giving the long east/west edges a planted transition to adjacent blocks.
    for (int32 Y = -18000; Y <= 18000; Y += 6000)
    {
        for (float X : {4200.0f, 15800.0f})
        {
            UStaticMesh* Tree = ((TreeIndex++ & 1) == 0) ? StreetTreeAMesh : StreetTreeBMesh;
            AddProductionVisual(Tree, FVector(X, static_cast<float>(Y), 0.0f), TEXT("SquareTree"));
            if (UStaticMeshComponent* Trunk = AddBox(
                FVector(X, static_cast<float>(Y), 420.0f),
                FVector(45.0f, 45.0f, 420.0f), TEXT("Vegetation_TrunkCollision")))
                Trunk->SetVisibility(false, true);
        }
    }

    const FVector CourtyardTrees[] = {
        FVector(-11800,7800,0), FVector(-13300,8300,0), FVector(-14350,10300,0),
        FVector(-11600,11000,0), FVector(-15100,7200,0)
    };
    for (const FVector& P : CourtyardTrees)
    {
        AddProductionVisual((TreeIndex++ & 1) ? StreetTreeAMesh : StreetTreeBMesh, P, TEXT("CourtyardTree"));
        if (UStaticMeshComponent* Trunk = AddBox(P + FVector(0,0,420),
            FVector(45,45,420), TEXT("Vegetation_TrunkCollision")))
            Trunk->SetVisibility(false, true);
    }
}

void ATU_DonetskDistrictGenerator::BuildMissionDamageLayer()
{
    // This layer is an original raid-state fiction, not a claim about the exact
    // damage state of any referenced building. Keep it spatially separate from
    // the calibrated architecture so it can be replaced per mission/era.
    struct FRubblePiece { FVector P; FVector E; FRotator R; };
    const FRubblePiece Rubble[] = {
        {FVector(-5200,-17600,42), FVector(170,95,42), FRotator(12,28,8)},
        {FVector(-4860,-17480,30), FVector(120,80,30), FRotator(-8,-14,5)},
        {FVector(15400,-14200,52), FVector(220,110,52), FRotator(18,42,-6)},
        {FVector(15820,-13950,34), FVector(135,90,34), FRotator(-12,5,9)},
        {FVector(5200,16600,38), FVector(180,105,38), FRotator(9,-31,7)},
        {FVector(5500,16950,26), FVector(105,70,26), FRotator(-4,17,-3)},
        {FVector(-3200,-23100,45), FVector(190,115,45), FRotator(14,36,5)},
        {FVector(3300,-23800,36), FVector(155,90,36), FRotator(-10,-22,8)}
    };
    for (const FRubblePiece& Piece : Rubble)
        AddHiddenBox(Piece.P, Piece.E, TEXT("Damage_RubbleConcrete"), Piece.R);

    // A small number of real debris meshes replace the visible cube read while the
    // hidden pieces above continue to own deterministic collision.
    AddProductionVisual(RubblePileMesh, FVector(-5200.0f, -17600.0f, 0.0f),
        TEXT("Damage_RubblePile_Production_A"), FRotator(0.0f, 28.0f, 0.0f));
    AddProductionVisual(RubblePileMesh, FVector(15400.0f, -14200.0f, 0.0f),
        TEXT("Damage_RubblePile_Production_B"), FRotator(0.0f, -18.0f, 0.0f));
    AddProductionVisual(RubblePileMesh, FVector(-3200.0f, -23100.0f, 0.0f),
        TEXT("Damage_RubblePile_Production_C"), FRotator(0.0f, 41.0f, 0.0f));

    // Improvised road-control positions create extraction-shooter cover without
    // blocking the documented Artema boulevard or the two extraction lanes.
    for (int32 I=0; I<4; ++I)
    {
        AddHiddenBox(FVector(-900.f + I*600.f, -20500.f + (I%2)*180.f, 55.f),
            FVector(210.f, 65.f, 55.f), TEXT("Damage_RubbleBarrier"),
            FRotator(0.f, I%2 ? 8.f : -6.f, 0.f));
    }

    AddHiddenBox(FVector(4300,-18700,125), FVector(290,20,125), TEXT("Damage_RustSheet"), FRotator(0,22,4));
    AddHiddenBox(FVector(4700,-18450,95), FVector(220,18,95), TEXT("Damage_RustSheet"), FRotator(0,-17,-3));
    AddHiddenBox(FVector(-18400,-12600,110), FVector(250,18,110), TEXT("Damage_RustSheet"), FRotator(0,9,5));
}

void ATU_DonetskDistrictGenerator::BeginPlay()
{
    // PostNetInit invokes BeginPlay after the initial authoritative layout properties arrive.
    // Spawned replication actors do not receive the server's dynamically generated components.
    if (!HasAuthority()) { bInitialLayoutReceived = true; OnConstruction(GetActorTransform()); }
    Super::BeginPlay();
}

void ATU_DonetskDistrictGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ATU_DonetskDistrictGenerator, DistrictWidthCm, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATU_DonetskDistrictGenerator, DistrictLengthCm, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATU_DonetskDistrictGenerator, bGenerateReferenceLabels, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATU_DonetskDistrictGenerator, bGenerateTransitFurniture, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATU_DonetskDistrictGenerator, bGenerateUrbanVegetation, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATU_DonetskDistrictGenerator, bGenerateMissionDamage, COND_InitialOnly);
}

int32 ATU_DonetskDistrictGenerator::GetGeneratedCollisionComponentCount() const
{
    int32 Count = 0;
    for (const UActorComponent* Component : GeneratedComponents)
        if (const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
            if (Mesh->IsRegistered() && Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision) ++Count;
    return Count;
}

FString ATU_DonetskDistrictGenerator::GetGeneratedGeometrySignature() const
{
    TArray<FString> Records;
    for (const UActorComponent* Component : GeneratedComponents)
    {
        const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
        if (!Mesh || Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        {
            continue;
        }

        Records.Add(
            Mesh->GetName()
            + TEXT("|")
            + Mesh->GetRelativeTransform().ToString()
            + TEXT("|")
            + Mesh->GetCollisionProfileName().ToString());
    }

    Records.Sort();
    const FString Joined = FString::Join(Records, TEXT(";"));
    return FString::Printf(TEXT("%d:%08x"), Records.Num(), FCrc::StrCrc32(*Joined));
}

int32 ATU_DonetskDistrictGenerator::GetProductionVisualComponentCount() const
{
    int32 Count = 0;
    for (const UActorComponent* Component : GeneratedComponents)
    {
        const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
        if (!Mesh || !Mesh->IsRegistered() || !Mesh->GetStaticMesh())
        {
            continue;
        }

        if (Mesh->GetStaticMesh() != CubeMesh
            && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        {
            ++Count;
        }
    }
    return Count;
}

int32 ATU_DonetskDistrictGenerator::GetVisiblePrimitiveFallbackCount() const
{
    static const TCHAR* IntentionalSurfaceTokens[] = {
        TEXT("DistrictGround"),
        TEXT("MainBoulevard"),
        TEXT("BoulevardMedian"),
        TEXT("CrossStreetSouth"),
        TEXT("CrossStreetNorth"),
        TEXT("WestSidewalk"),
        TEXT("EastSidewalk"),
        TEXT("CentralSquare_Hardscape"),
        TEXT("CentralSquare_WestApron"),
        TEXT("CentralSquare_EastStreet"),
        TEXT("CentralSquare_SouthParterre"),
        TEXT("CentralSquare_NorthParterre"),
        TEXT("Khrush_CourtyardHardscape"),
        TEXT("Khrush_PlaygroundPad"),
        TEXT("Industrial_ServiceYard"),
    };

    int32 Count = 0;
    for (const UActorComponent* Component : GeneratedComponents)
    {
        const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
        if (!Mesh || !Mesh->IsRegistered() || !Mesh->IsVisible()
            || Mesh->GetStaticMesh() != CubeMesh)
        {
            continue;
        }

        const FString ComponentName = Mesh->GetName();
        bool bIntentionalSurfaceCarrier = false;
        for (const TCHAR* Token : IntentionalSurfaceTokens)
        {
            if (ComponentName.Contains(Token))
            {
                bIntentionalSurfaceCarrier = true;
                break;
            }
        }

        if (!bIntentionalSurfaceCarrier)
        {
            ++Count;
        }
    }
    return Count;
}

FString ATU_DonetskDistrictGenerator::GetGeneratedVisualSignature() const
{
    TArray<FString> Records;
    for (const UActorComponent* Component : GeneratedComponents)
    {
        const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
        UStaticMesh* StaticMesh = Mesh ? Mesh->GetStaticMesh() : nullptr;
        if (!Mesh || !Mesh->IsRegistered() || !StaticMesh
            || StaticMesh == CubeMesh
            || Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
        {
            continue;
        }

        Records.Add(
            StaticMesh->GetPathName()
            + TEXT("|")
            + Mesh->GetRelativeTransform().ToString());
    }

    Records.Sort();
    const FString Joined = FString::Join(Records, TEXT(";"));
    return FString::Printf(TEXT("%d:%08x"), Records.Num(), FCrc::StrCrc32(*Joined));
}
