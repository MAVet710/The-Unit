#include "TU_KillhouseGenerator.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATU_KillhouseGenerator::ATU_KillhouseGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeMeshFinder.Object;
}

void ATU_KillhouseGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ClearGeneratedComponents();
    GeneratedNameCounter = 0;
    BuildKillhouse();
}

void ATU_KillhouseGenerator::BuildKillhouse()
{
    if (!CubeMesh)
    {
        return;
    }

    GenerateFloor();
    GeneratePerimeterWalls();
    GenerateReferenceRoomLayout();

    if (bGenerateWallPosts)
    {
        GenerateWallPosts();
    }

    GenerateCenterStair();

    if (bGenerateCatwalk)
    {
        GenerateCrossCatwalk();

        if (bGenerateExteriorStair)
        {
            GenerateExteriorStair();
        }
    }

    if (bGenerateRoof)
    {
        const float RoofZ = CatwalkHeight + RailHeight + 150.0f;
        AddCube(
            FVector(0.0f, 0.0f, RoofZ),
            FVector(BuildingWidth * 0.5f, BuildingLength * 0.5f, 10.0f),
            TEXT("OptionalRoof"),
            FloorMaterial);
    }

    const float HalfL = BuildingLength * 0.5f;

    AddMarker(
        FVector(0.0f, -HalfL - 220.0f, 40.0f),
        FRotator::ZeroRotator,
        FLinearColor::Green,
        TEXT("PlayerStartMarker"));

    AddMarker(
        FVector(0.0f, BuildingLength * 0.33f, 40.0f),
        FRotator::ZeroRotator,
        FLinearColor::Yellow,
        TEXT("ObjectiveMarker"));

    AddMarker(
        FVector(0.0f, HalfL + 140.0f, 40.0f),
        FRotator(0.0f, 180.0f, 0.0f),
        FLinearColor::Blue,
        TEXT("ExtractionMarker"));

    const TArray<FVector> EnemySpawnLocations =
    {
        FVector(-BuildingWidth * 0.33f, -BuildingLength * 0.31f, 40.0f),
        FVector(BuildingWidth * 0.33f, -BuildingLength * 0.31f, 40.0f),
        FVector(-BuildingWidth * 0.34f, -BuildingLength * 0.03f, 40.0f),
        FVector(BuildingWidth * 0.34f, -BuildingLength * 0.02f, 40.0f),
        FVector(-BuildingWidth * 0.28f, BuildingLength * 0.31f, 40.0f),
        FVector(BuildingWidth * 0.28f, BuildingLength * 0.31f, 40.0f)
    };

    for (int32 Index = 0; Index < EnemySpawnLocations.Num(); ++Index)
    {
        AddMarker(
            EnemySpawnLocations[Index],
            FRotator::ZeroRotator,
            FLinearColor::Red,
            FString::Printf(TEXT("EnemySpawn_%d"), Index + 1));
    }

    AddMarker(
        FVector(-BuildingWidth * 0.18f, -BuildingLength * 0.08f, 40.0f),
        FRotator::ZeroRotator,
        FLinearColor::Cyan,
        TEXT("PatrolPoint_A"));

    AddMarker(
        FVector(BuildingWidth * 0.18f, BuildingLength * 0.18f, 40.0f),
        FRotator::ZeroRotator,
        FLinearColor::Cyan,
        TEXT("PatrolPoint_B"));

    if (bGenerateDebugLabels)
    {
        SpawnDebugLabel(TEXT("ENTRY"), FVector(0.0f, -HalfL - 140.0f, 120.0f));
        SpawnDebugLabel(TEXT("OBJECTIVE"), FVector(0.0f, BuildingLength * 0.33f, 120.0f));
        SpawnDebugLabel(TEXT("EXTRACT"), FVector(0.0f, HalfL + 100.0f, 120.0f));
    }
}

UStaticMeshComponent* ATU_KillhouseGenerator::AddCube(
    const FVector& Location,
    const FVector& Extents,
    const FString& BaseName,
    UMaterialInterface* Material,
    const FRotator& Rotation)
{
    UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this, MakeGeneratedName(BaseName));
    MeshComponent->SetStaticMesh(CubeMesh);
    MeshComponent->SetRelativeLocation(Location);
    MeshComponent->SetRelativeRotation(Rotation);
    MeshComponent->SetRelativeScale3D(Extents / 50.0f);
    MeshComponent->SetMobility(EComponentMobility::Static);
    MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    MeshComponent->SetCanEverAffectNavigation(true);

    if (Material)
    {
        MeshComponent->SetMaterial(0, Material);
    }

    MeshComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    MeshComponent->RegisterComponent();

    GeneratedComponents.Add(MeshComponent);
    return MeshComponent;
}

UArrowComponent* ATU_KillhouseGenerator::AddMarker(
    const FVector& Location,
    const FRotator& Rotation,
    const FLinearColor& Color,
    const FString& BaseName)
{
    UArrowComponent* Marker = NewObject<UArrowComponent>(this, MakeGeneratedName(BaseName));
    Marker->ArrowColor = Color.ToFColor(true);
    Marker->ArrowSize = 1.5f;
    Marker->SetRelativeLocation(Location);
    Marker->SetRelativeRotation(Rotation);
    Marker->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Marker->RegisterComponent();

    GeneratedComponents.Add(Marker);
    return Marker;
}

void ATU_KillhouseGenerator::GenerateFloor()
{
    const float HalfW = BuildingWidth * 0.5f;
    const float HalfL = BuildingLength * 0.5f;
    const float SlabHalfW = HalfW + StairWidth + 140.0f;
    const float SlabHalfL = HalfL + 140.0f;

    AddCube(
        FVector(0.0f, 0.0f, -10.0f),
        FVector(SlabHalfW, SlabHalfL, 10.0f),
        TEXT("BaseSlab"),
        FloorMaterial);

    AddCube(
        FVector(0.0f, -HalfL - 300.0f, -8.0f),
        FVector(BuildingWidth * 0.32f, 300.0f, 8.0f),
        TEXT("EntryApron"),
        FloorMaterial);
}

void ATU_KillhouseGenerator::GeneratePerimeterWalls()
{
    const float HalfW = BuildingWidth * 0.5f;
    const float HalfL = BuildingLength * 0.5f;
    const float WallZ = FloorHeight * 0.5f;

    // Front and rear walls both contain doors so the course can be run in either direction.
    AddWallWithDoorX(-HalfL, -HalfW, HalfW, 0.0f, TEXT("FrontWall"));
    AddWallWithDoorX(HalfL, -HalfW, HalfW, -BuildingWidth * 0.23f, TEXT("RearWall"));

    // Side doors support alternate entries and scenario variation.
    AddWallWithDoorY(-HalfW, -HalfL, HalfL, -BuildingLength * 0.22f, TEXT("LeftWall"));
    AddWallWithDoorY(HalfW, -HalfL, HalfL, BuildingLength * 0.27f, TEXT("RightWall"));

    // Reinforce the four corners visually so the open-top shell reads as a modular structure.
    const float Corner = WallThickness * 0.5f;
    AddCube(FVector(-HalfW, -HalfL, WallZ), FVector(Corner, Corner, FloorHeight * 0.5f), TEXT("Corner_SW"), WallMaterial);
    AddCube(FVector(HalfW, -HalfL, WallZ), FVector(Corner, Corner, FloorHeight * 0.5f), TEXT("Corner_SE"), WallMaterial);
    AddCube(FVector(-HalfW, HalfL, WallZ), FVector(Corner, Corner, FloorHeight * 0.5f), TEXT("Corner_NW"), WallMaterial);
    AddCube(FVector(HalfW, HalfL, WallZ), FVector(Corner, Corner, FloorHeight * 0.5f), TEXT("Corner_NE"), WallMaterial);
}

void ATU_KillhouseGenerator::GenerateReferenceRoomLayout()
{
    const float HalfW = BuildingWidth * 0.5f;
    const float HalfL = BuildingLength * 0.5f;
    const float CrossY = BuildingLength * 0.12f;
    const float CatwalkFrontY = CrossY - (CatwalkWidth * 0.5f);
    const float BackFrontY = CrossY + (CatwalkWidth * 0.52f);
    const float WallZ = FloorHeight * 0.5f;

    // Three larger rooms across the rear half, matching the reference's long back row.
    const float RearDividerX = BuildingWidth / 6.0f;

    AddWallWithDoorX(
        BackFrontY,
        -HalfW,
        -RearDividerX,
        (-HalfW - RearDividerX) * 0.5f,
        TEXT("RearLeftFront"));

    AddWallWithDoorX(
        BackFrontY,
        -RearDividerX,
        RearDividerX,
        0.0f,
        TEXT("RearCenterFront"));

    AddWallWithDoorX(
        BackFrontY,
        RearDividerX,
        HalfW,
        (HalfW + RearDividerX) * 0.5f,
        TEXT("RearRightFront"));

    const float RearPartitionLength = HalfL - BackFrontY;
    const float RearPartitionCenterY = BackFrontY + (RearPartitionLength * 0.5f);

    AddCube(
        FVector(-RearDividerX, RearPartitionCenterY, WallZ),
        FVector(WallThickness * 0.5f, RearPartitionLength * 0.5f, FloorHeight * 0.5f),
        TEXT("RearDivider_Left"),
        WallMaterial);

    AddCube(
        FVector(RearDividerX, RearPartitionCenterY, WallZ),
        FVector(WallThickness * 0.5f, RearPartitionLength * 0.5f, FloorHeight * 0.5f),
        TEXT("RearDivider_Right"),
        WallMaterial);

    // The center is kept open for the broad stair seen in the reference model.
    const float StairBayHalf = (StairWidth * 0.5f) + 115.0f;
    const float SplitY = -BuildingLength * 0.16f;

    AddWallWithDoorY(
        -StairBayHalf,
        -HalfL,
        SplitY,
        -BuildingLength * 0.32f,
        TEXT("CenterLeftLower"));

    AddWallWithDoorY(
        -StairBayHalf,
        SplitY,
        CatwalkFrontY,
        -BuildingLength * 0.03f,
        TEXT("CenterLeftUpper"));

    AddWallWithDoorY(
        StairBayHalf,
        -HalfL,
        SplitY,
        -BuildingLength * 0.29f,
        TEXT("CenterRightLower"));

    AddWallWithDoorY(
        StairBayHalf,
        SplitY,
        CatwalkFrontY,
        BuildingLength * 0.01f,
        TEXT("CenterRightUpper"));

    // Front wings are intentionally asymmetric to create different room-entry problems.
    const float FrontCrossY = -BuildingLength * 0.20f;

    AddWallWithDoorX(
        FrontCrossY,
        -HalfW,
        -StairBayHalf,
        -BuildingWidth * 0.34f,
        TEXT("FrontLeftCrossWall"));

    AddWallWithDoorX(
        FrontCrossY,
        StairBayHalf,
        HalfW,
        BuildingWidth * 0.31f,
        TEXT("FrontRightCrossWall"));

    const float LeftWingDividerX = -BuildingWidth * 0.34f;
    const float RightWingDividerX = BuildingWidth * 0.34f;

    AddWallWithDoorY(
        LeftWingDividerX,
        FrontCrossY,
        CatwalkFrontY,
        -BuildingLength * 0.02f,
        TEXT("LeftWingDivider"));

    AddWallWithDoorY(
        RightWingDividerX,
        FrontCrossY,
        CatwalkFrontY,
        BuildingLength * 0.015f,
        TEXT("RightWingDivider"));

    // One extra offset wall creates the smaller front-right room visible in the reference.
    const float SmallRoomY = -BuildingLength * 0.035f;
    AddWallWithDoorX(
        SmallRoomY,
        RightWingDividerX,
        HalfW,
        BuildingWidth * 0.42f,
        TEXT("RightSmallRoomCrossWall"));

    if (bGenerateDebugLabels)
    {
        SpawnDebugLabel(TEXT("REAR L"), FVector(-BuildingWidth * 0.33f, BuildingLength * 0.31f, 120.0f));
        SpawnDebugLabel(TEXT("REAR C"), FVector(0.0f, BuildingLength * 0.31f, 120.0f));
        SpawnDebugLabel(TEXT("REAR R"), FVector(BuildingWidth * 0.33f, BuildingLength * 0.31f, 120.0f));
        SpawnDebugLabel(TEXT("STAIR BAY"), FVector(0.0f, -BuildingLength * 0.12f, 120.0f));
    }
}

void ATU_KillhouseGenerator::GenerateCenterStair()
{
    const int32 StepCount = FMath::Clamp(StairStepCount, 8, 20);
    const float CrossY = BuildingLength * 0.12f;
    const float TopY = CrossY - (CatwalkWidth * 0.5f);
    const float MaxRun = BuildingLength * 0.46f;
    const float Run = FMath::Min(CenterStairRun, MaxRun);
    const float StartY = TopY - Run;
    const float StepDepth = Run / static_cast<float>(StepCount);
    const float StepHeight = CatwalkHeight / static_cast<float>(StepCount);

    for (int32 Index = 0; Index < StepCount; ++Index)
    {
        const float StepTopZ = (Index + 1) * StepHeight;
        const float StepY = StartY + ((Index + 0.5f) * StepDepth);

        AddCube(
            FVector(0.0f, StepY, StepTopZ - (StepHeight * 0.5f)),
            FVector(StairWidth * 0.5f, StepDepth * 0.5f, StepHeight * 0.5f),
            FString::Printf(TEXT("CenterStairStep_%02d"), Index),
            MetalMaterial);

        if ((Index % 2) == 0 || Index == StepCount - 1)
        {
            const float PostZ = StepTopZ + (RailHeight * 0.5f);
            const float SideX = (StairWidth * 0.5f) + (RailThickness * 0.5f);

            AddCube(
                FVector(-SideX, StepY, PostZ),
                FVector(RailThickness * 0.5f, RailThickness * 0.5f, RailHeight * 0.5f),
                FString::Printf(TEXT("CenterStairPost_L_%02d"), Index),
                MetalMaterial);

            AddCube(
                FVector(SideX, StepY, PostZ),
                FVector(RailThickness * 0.5f, RailThickness * 0.5f, RailHeight * 0.5f),
                FString::Printf(TEXT("CenterStairPost_R_%02d"), Index),
                MetalMaterial);
        }
    }

    const float RailLength = FMath::Sqrt((Run * Run) + (CatwalkHeight * CatwalkHeight));
    const float RailRoll = FMath::RadiansToDegrees(FMath::Atan2(CatwalkHeight, Run));
    const float RailMidY = (StartY + TopY) * 0.5f;
    const float SideX = (StairWidth * 0.5f) + (RailThickness * 0.5f);

    for (int32 RailLevel = 0; RailLevel < 2; ++RailLevel)
    {
        const float HeightFactor = RailLevel == 0 ? 0.52f : 0.95f;
        const float RailMidZ = (CatwalkHeight * 0.5f) + (RailHeight * HeightFactor);

        AddCube(
            FVector(-SideX, RailMidY, RailMidZ),
            FVector(RailThickness * 0.5f, RailLength * 0.5f, RailThickness * 0.5f),
            FString::Printf(TEXT("CenterStairRail_L_%d"), RailLevel),
            MetalMaterial,
            FRotator(0.0f, 0.0f, RailRoll));

        AddCube(
            FVector(SideX, RailMidY, RailMidZ),
            FVector(RailThickness * 0.5f, RailLength * 0.5f, RailThickness * 0.5f),
            FString::Printf(TEXT("CenterStairRail_R_%d"), RailLevel),
            MetalMaterial,
            FRotator(0.0f, 0.0f, RailRoll));
    }
}

void ATU_KillhouseGenerator::GenerateCrossCatwalk()
{
    const float HalfW = BuildingWidth * 0.5f;
    const float CrossY = BuildingLength * 0.12f;
    const float DeckTop = CatwalkHeight + (CatwalkThickness * 0.5f);
    const float FrontY = CrossY - (CatwalkWidth * 0.5f);
    const float RearY = CrossY + (CatwalkWidth * 0.5f);

    AddCube(
        FVector(0.0f, CrossY, CatwalkHeight),
        FVector(HalfW, CatwalkWidth * 0.5f, CatwalkThickness * 0.5f),
        TEXT("CrossCatwalkDeck"),
        MetalMaterial);

    // Small exterior landing at the right end, as seen in the reference model.
    AddCube(
        FVector(HalfW + (StairWidth * 0.5f), CrossY, CatwalkHeight),
        FVector(StairWidth * 0.5f, CatwalkWidth * 0.5f, CatwalkThickness * 0.5f),
        TEXT("ExteriorCatwalkLanding"),
        MetalMaterial);

    const float CenterGapHalf = (StairWidth * 0.5f) + 28.0f;

    GenerateGuardRailAlongX(-HalfW, -CenterGapHalf, FrontY, DeckTop, TEXT("CatwalkFrontLeft"));
    GenerateGuardRailAlongX(CenterGapHalf, HalfW, FrontY, DeckTop, TEXT("CatwalkFrontRight"));
    GenerateGuardRailAlongX(-HalfW, HalfW + StairWidth, RearY, DeckTop, TEXT("CatwalkRear"));
    GenerateGuardRailAlongY(FrontY, RearY, -HalfW, DeckTop, TEXT("CatwalkWestEnd"));
    GenerateGuardRailAlongY(FrontY, RearY, HalfW + StairWidth, DeckTop, TEXT("CatwalkEastEnd"));

    // Under-deck beams make the bridge feel structural even before final art is added.
    const int32 BeamCount = 7;
    for (int32 Index = 0; Index < BeamCount; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(BeamCount - 1);
        const float X = FMath::Lerp(-HalfW + 120.0f, HalfW - 120.0f, Alpha);

        AddCube(
            FVector(X, CrossY, CatwalkHeight - CatwalkThickness - 12.0f),
            FVector(8.0f, CatwalkWidth * 0.5f, 12.0f),
            FString::Printf(TEXT("CatwalkSupportBeam_%02d"), Index),
            MetalMaterial);
    }
}

void ATU_KillhouseGenerator::GenerateExteriorStair()
{
    const int32 StepCount = FMath::Clamp(StairStepCount, 8, 20);
    const float HalfW = BuildingWidth * 0.5f;
    const float CrossY = BuildingLength * 0.12f;
    const float TopY = CrossY - (CatwalkWidth * 0.5f);
    const float Run = FMath::Min(ExteriorStairRun, BuildingLength * 0.42f);
    const float BottomY = TopY - Run;
    const float StepDepth = Run / static_cast<float>(StepCount);
    const float StepHeight = CatwalkHeight / static_cast<float>(StepCount);
    const float StairCenterX = HalfW + (StairWidth * 0.5f);

    for (int32 Index = 0; Index < StepCount; ++Index)
    {
        const float StepTopZ = (Index + 1) * StepHeight;
        const float StepY = BottomY + ((Index + 0.5f) * StepDepth);

        AddCube(
            FVector(StairCenterX, StepY, StepTopZ - (StepHeight * 0.5f)),
            FVector(StairWidth * 0.5f, StepDepth * 0.5f, StepHeight * 0.5f),
            FString::Printf(TEXT("ExteriorStairStep_%02d"), Index),
            MetalMaterial);

        if ((Index % 2) == 0 || Index == StepCount - 1)
        {
            const float PostZ = StepTopZ + (RailHeight * 0.5f);
            const float LeftX = StairCenterX - (StairWidth * 0.5f);
            const float RightX = StairCenterX + (StairWidth * 0.5f);

            AddCube(
                FVector(LeftX, StepY, PostZ),
                FVector(RailThickness * 0.5f, RailThickness * 0.5f, RailHeight * 0.5f),
                FString::Printf(TEXT("ExteriorStairPost_L_%02d"), Index),
                MetalMaterial);

            AddCube(
                FVector(RightX, StepY, PostZ),
                FVector(RailThickness * 0.5f, RailThickness * 0.5f, RailHeight * 0.5f),
                FString::Printf(TEXT("ExteriorStairPost_R_%02d"), Index),
                MetalMaterial);
        }
    }

    const float RailLength = FMath::Sqrt((Run * Run) + (CatwalkHeight * CatwalkHeight));
    const float RailRoll = FMath::RadiansToDegrees(FMath::Atan2(CatwalkHeight, Run));
    const float RailMidY = (BottomY + TopY) * 0.5f;
    const float LeftX = StairCenterX - (StairWidth * 0.5f);
    const float RightX = StairCenterX + (StairWidth * 0.5f);

    for (int32 RailLevel = 0; RailLevel < 2; ++RailLevel)
    {
        const float HeightFactor = RailLevel == 0 ? 0.52f : 0.95f;
        const float RailMidZ = (CatwalkHeight * 0.5f) + (RailHeight * HeightFactor);

        AddCube(
            FVector(LeftX, RailMidY, RailMidZ),
            FVector(RailThickness * 0.5f, RailLength * 0.5f, RailThickness * 0.5f),
            FString::Printf(TEXT("ExteriorStairRail_L_%d"), RailLevel),
            MetalMaterial,
            FRotator(0.0f, 0.0f, RailRoll));

        AddCube(
            FVector(RightX, RailMidY, RailMidZ),
            FVector(RailThickness * 0.5f, RailLength * 0.5f, RailThickness * 0.5f),
            FString::Printf(TEXT("ExteriorStairRail_R_%d"), RailLevel),
            MetalMaterial,
            FRotator(0.0f, 0.0f, RailRoll));
    }
}

void ATU_KillhouseGenerator::GenerateWallPosts()
{
    const float HalfW = BuildingWidth * 0.5f;
    const float HalfL = BuildingLength * 0.5f;
    const float CrossY = BuildingLength * 0.12f;
    const float BackFrontY = CrossY + (CatwalkWidth * 0.52f);
    const float RearDividerX = BuildingWidth / 6.0f;
    const float PostHeight = 120.0f;
    const float PostZ = FloorHeight + (PostHeight * 0.5f);

    const TArray<FVector2D> PostLocations =
    {
        FVector2D(-HalfW, -HalfL), FVector2D(0.0f, -HalfL), FVector2D(HalfW, -HalfL),
        FVector2D(-HalfW, 0.0f), FVector2D(HalfW, 0.0f),
        FVector2D(-HalfW, HalfL), FVector2D(0.0f, HalfL), FVector2D(HalfW, HalfL),
        FVector2D(-RearDividerX, BackFrontY), FVector2D(RearDividerX, BackFrontY),
        FVector2D(-RearDividerX, HalfL), FVector2D(RearDividerX, HalfL)
    };

    for (int32 Index = 0; Index < PostLocations.Num(); ++Index)
    {
        AddCube(
            FVector(PostLocations[Index].X, PostLocations[Index].Y, PostZ),
            FVector(7.0f, 7.0f, PostHeight * 0.5f),
            FString::Printf(TEXT("WallPost_%02d"), Index),
            MetalMaterial);
    }
}

void ATU_KillhouseGenerator::AddWallWithDoorX(
    float Y,
    float XMin,
    float XMax,
    float DoorCenterX,
    const FString& Prefix)
{
    if (XMax <= XMin)
    {
        return;
    }

    const float SegmentLength = XMax - XMin;
    const float DoorHalf = FMath::Min(DoorWidth * 0.5f, SegmentLength * 0.42f);
    const float DoorCenter = FMath::Clamp(DoorCenterX, XMin + DoorHalf, XMax - DoorHalf);
    const float LeftEnd = DoorCenter - DoorHalf;
    const float RightStart = DoorCenter + DoorHalf;
    const float WallZ = FloorHeight * 0.5f;

    if (LeftEnd > XMin + 1.0f)
    {
        AddCube(
            FVector((XMin + LeftEnd) * 0.5f, Y, WallZ),
            FVector((LeftEnd - XMin) * 0.5f, WallThickness * 0.5f, FloorHeight * 0.5f),
            Prefix + TEXT("_Left"),
            WallMaterial);
    }

    if (XMax > RightStart + 1.0f)
    {
        AddCube(
            FVector((RightStart + XMax) * 0.5f, Y, WallZ),
            FVector((XMax - RightStart) * 0.5f, WallThickness * 0.5f, FloorHeight * 0.5f),
            Prefix + TEXT("_Right"),
            WallMaterial);
    }

    const float EffectiveDoorHeight = FMath::Min(DoorHeight, FloorHeight - 10.0f);
    const float LintelHeight = FloorHeight - EffectiveDoorHeight;

    if (LintelHeight > 1.0f)
    {
        AddCube(
            FVector(DoorCenter, Y, EffectiveDoorHeight + (LintelHeight * 0.5f)),
            FVector(DoorHalf, WallThickness * 0.5f, LintelHeight * 0.5f),
            Prefix + TEXT("_Lintel"),
            WallMaterial);
    }
}

void ATU_KillhouseGenerator::AddWallWithDoorY(
    float X,
    float YMin,
    float YMax,
    float DoorCenterY,
    const FString& Prefix)
{
    if (YMax <= YMin)
    {
        return;
    }

    const float SegmentLength = YMax - YMin;
    const float DoorHalf = FMath::Min(DoorWidth * 0.5f, SegmentLength * 0.42f);
    const float DoorCenter = FMath::Clamp(DoorCenterY, YMin + DoorHalf, YMax - DoorHalf);
    const float LowerEnd = DoorCenter - DoorHalf;
    const float UpperStart = DoorCenter + DoorHalf;
    const float WallZ = FloorHeight * 0.5f;

    if (LowerEnd > YMin + 1.0f)
    {
        AddCube(
            FVector(X, (YMin + LowerEnd) * 0.5f, WallZ),
            FVector(WallThickness * 0.5f, (LowerEnd - YMin) * 0.5f, FloorHeight * 0.5f),
            Prefix + TEXT("_Lower"),
            WallMaterial);
    }

    if (YMax > UpperStart + 1.0f)
    {
        AddCube(
            FVector(X, (UpperStart + YMax) * 0.5f, WallZ),
            FVector(WallThickness * 0.5f, (YMax - UpperStart) * 0.5f, FloorHeight * 0.5f),
            Prefix + TEXT("_Upper"),
            WallMaterial);
    }

    const float EffectiveDoorHeight = FMath::Min(DoorHeight, FloorHeight - 10.0f);
    const float LintelHeight = FloorHeight - EffectiveDoorHeight;

    if (LintelHeight > 1.0f)
    {
        AddCube(
            FVector(X, DoorCenter, EffectiveDoorHeight + (LintelHeight * 0.5f)),
            FVector(WallThickness * 0.5f, DoorHalf, LintelHeight * 0.5f),
            Prefix + TEXT("_Lintel"),
            WallMaterial);
    }
}

void ATU_KillhouseGenerator::GenerateGuardRailAlongX(
    float XMin,
    float XMax,
    float Y,
    float ZBase,
    const FString& Prefix)
{
    const float Length = XMax - XMin;
    if (Length <= RailThickness)
    {
        return;
    }

    const float MidX = (XMin + XMax) * 0.5f;

    AddCube(
        FVector(MidX, Y, ZBase + RailHeight),
        FVector(Length * 0.5f, RailThickness * 0.5f, RailThickness * 0.5f),
        Prefix + TEXT("_TopRail"),
        MetalMaterial);

    AddCube(
        FVector(MidX, Y, ZBase + (RailHeight * 0.52f)),
        FVector(Length * 0.5f, RailThickness * 0.5f, RailThickness * 0.5f),
        Prefix + TEXT("_MidRail"),
        MetalMaterial);

    const int32 PostSegments = FMath::Max(1, FMath::CeilToInt(Length / 280.0f));
    for (int32 Index = 0; Index <= PostSegments; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(PostSegments);
        const float X = FMath::Lerp(XMin, XMax, Alpha);

        AddCube(
            FVector(X, Y, ZBase + (RailHeight * 0.5f)),
            FVector(RailThickness * 0.5f, RailThickness * 0.5f, RailHeight * 0.5f),
            FString::Printf(TEXT("%s_Post_%02d"), *Prefix, Index),
            MetalMaterial);
    }
}

void ATU_KillhouseGenerator::GenerateGuardRailAlongY(
    float YMin,
    float YMax,
    float X,
    float ZBase,
    const FString& Prefix)
{
    const float Length = YMax - YMin;
    if (Length <= RailThickness)
    {
        return;
    }

    const float MidY = (YMin + YMax) * 0.5f;

    AddCube(
        FVector(X, MidY, ZBase + RailHeight),
        FVector(RailThickness * 0.5f, Length * 0.5f, RailThickness * 0.5f),
        Prefix + TEXT("_TopRail"),
        MetalMaterial);

    AddCube(
        FVector(X, MidY, ZBase + (RailHeight * 0.52f)),
        FVector(RailThickness * 0.5f, Length * 0.5f, RailThickness * 0.5f),
        Prefix + TEXT("_MidRail"),
        MetalMaterial);

    const int32 PostSegments = FMath::Max(1, FMath::CeilToInt(Length / 280.0f));
    for (int32 Index = 0; Index <= PostSegments; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(PostSegments);
        const float Y = FMath::Lerp(YMin, YMax, Alpha);

        AddCube(
            FVector(X, Y, ZBase + (RailHeight * 0.5f)),
            FVector(RailThickness * 0.5f, RailThickness * 0.5f, RailHeight * 0.5f),
            FString::Printf(TEXT("%s_Post_%02d"), *Prefix, Index),
            MetalMaterial);
    }
}

void ATU_KillhouseGenerator::SpawnDebugLabel(const FString& Label, const FVector& Location)
{
    UTextRenderComponent* TextComponent = NewObject<UTextRenderComponent>(this, MakeGeneratedName(TEXT("DebugLabel")));
    TextComponent->SetText(FText::FromString(Label));
    TextComponent->SetHorizontalAlignment(EHTA_Center);
    TextComponent->SetWorldSize(42.0f);
    TextComponent->SetTextRenderColor(FColor::White);
    TextComponent->SetRelativeLocation(Location);
    TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    TextComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    TextComponent->RegisterComponent();

    GeneratedComponents.Add(TextComponent);
}

void ATU_KillhouseGenerator::ClearGeneratedComponents()
{
    for (UActorComponent* Component : GeneratedComponents)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }

    GeneratedComponents.Reset();
}

FName ATU_KillhouseGenerator::MakeGeneratedName(const FString& BaseName)
{
    return FName(*FString::Printf(TEXT("%s_%04d"), *BaseName, GeneratedNameCounter++));
}
