#include "TU_KillhouseGenerator.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
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
    BuildKillhouse();
}

void ATU_KillhouseGenerator::BuildKillhouse()
{
    if (!CubeMesh)
    {
        return;
    }

    const int32 FirstRooms = FMath::Clamp(FirstFloorRoomCount, 3, 8);

    GenerateFloor(0.0f);
    GenerateWalls(0.0f);

    const float RoomDepth = BuildingLength * 0.3f;
    const float LeftRoomWidth = (BuildingWidth - HallwayWidth) * 0.5f;

    GenerateRoom(FVector(-BuildingWidth * 0.25f, -BuildingLength * 0.3f, 0.0f), FVector2D(LeftRoomWidth, RoomDepth), 0.0f, TEXT("Lobby"), bGenerateDebugLabels, true, false);
    GenerateRoom(FVector(-BuildingWidth * 0.25f, BuildingLength * 0.05f, 0.0f), FVector2D(LeftRoomWidth, RoomDepth), 0.0f, TEXT("Storage"), bGenerateDebugLabels, true, false);
    GenerateRoom(FVector(BuildingWidth * 0.25f, -BuildingLength * 0.22f, 0.0f), FVector2D(LeftRoomWidth, RoomDepth * 0.8f), 0.0f, TEXT("Stairwell"), bGenerateDebugLabels, true, true);

    if (FirstRooms >= 4)
    {
        GenerateRoom(FVector(-BuildingWidth * 0.25f, BuildingLength * 0.35f, 0.0f), FVector2D(LeftRoomWidth, RoomDepth), 0.0f, TEXT("Office"), bGenerateDebugLabels, true, false);
    }
    if (FirstRooms >= 5)
    {
        GenerateRoom(FVector(BuildingWidth * 0.25f, BuildingLength * 0.12f, 0.0f), FVector2D(LeftRoomWidth, RoomDepth), 0.0f, TEXT("Utility"), false, true, true);
    }
    if (FirstRooms >= 6)
    {
        GenerateRoom(FVector(BuildingWidth * 0.25f, BuildingLength * 0.38f, 0.0f), FVector2D(LeftRoomWidth, RoomDepth * 0.7f), 0.0f, TEXT("Objective Staging"), false, true, true);
    }

    GenerateStairs(0.0f);
    GenerateSecondFloor(FloorHeight);

    if (bGenerateRoof)
    {
        AddCube(FVector(0.0f, 0.0f, FloorHeight * 2.0f), FVector(BuildingWidth * 0.5f, BuildingLength * 0.5f, 10.0f), TEXT("Roof"));
    }

    AddMarker(FVector(-200.0f, -BuildingLength * 0.7f, 40.0f), FRotator(0.0f, 0.0f, 0.0f), FLinearColor::Green, TEXT("PlayerStartMarker"));
    AddMarker(FVector(BuildingWidth * 0.25f, BuildingLength * 0.25f, FloorHeight + 40.0f), FRotator::ZeroRotator, FLinearColor::Yellow, TEXT("ObjectiveMarker"));
    AddMarker(FVector(0.0f, BuildingLength * 0.75f, 40.0f), FRotator(0.0f, 180.0f, 0.0f), FLinearColor::Blue, TEXT("ExtractionMarker"));

    for (int32 Index = 0; Index < 6; ++Index)
    {
        const float X = (Index % 2 == 0) ? -BuildingWidth * 0.3f : BuildingWidth * 0.3f;
        const float Y = -BuildingLength * 0.15f + (Index * 220.0f);
        const float Z = (Index > 3) ? FloorHeight + 40.0f : 40.0f;
        AddMarker(FVector(X, Y, Z), FRotator::ZeroRotator, FLinearColor::Red, *FString::Printf(TEXT("EnemySpawn_%d"), Index + 1));
    }

    AddMarker(FVector(0.0f, -100.0f, 40.0f), FRotator::ZeroRotator, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), TEXT("PatrolPoint_A"));
    AddMarker(FVector(0.0f, 300.0f, FloorHeight + 40.0f), FRotator::ZeroRotator, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), TEXT("PatrolPoint_B"));

    if (bGenerateDebugLabels)
    {
        SpawnDebugLabel(TEXT("Extraction"), FVector(0.0f, BuildingLength * 0.75f, 120.0f));
    }
}

UStaticMeshComponent* ATU_KillhouseGenerator::AddCube(const FVector& Location, const FVector& Extents, const FName& Name, const FRotator& Rotation)
{
    UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this, Name);
    MeshComponent->SetStaticMesh(CubeMesh);
    MeshComponent->SetRelativeLocation(Location);
    MeshComponent->SetRelativeRotation(Rotation);
    MeshComponent->SetRelativeScale3D(Extents / 50.0f);
    MeshComponent->SetMobility(EComponentMobility::Static);
    MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    MeshComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    MeshComponent->RegisterComponent();

    GeneratedComponents.Add(MeshComponent);
    return MeshComponent;
}

UArrowComponent* ATU_KillhouseGenerator::AddMarker(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, const FName& Name)
{
    UArrowComponent* Marker = NewObject<UArrowComponent>(this, Name);
    Marker->ArrowColor = Color.ToFColor(true);
    Marker->ArrowSize = 1.5f;
    Marker->SetRelativeLocation(Location);
    Marker->SetRelativeRotation(Rotation);
    Marker->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Marker->RegisterComponent();

    GeneratedComponents.Add(Marker);
    return Marker;
}

void ATU_KillhouseGenerator::GenerateFloor(float ZOffset)
{
    AddCube(FVector(0.0f, 0.0f, ZOffset - 10.0f), FVector(BuildingWidth * 0.5f, BuildingLength * 0.5f, 10.0f), FName(*FString::Printf(TEXT("Floor_%d"), static_cast<int32>(ZOffset))));

    AddCube(FVector(0.0f, -BuildingLength * 0.8f, ZOffset - 10.0f), FVector(BuildingWidth * 0.4f, 350.0f, 10.0f), FName(*FString::Printf(TEXT("Staging_%d"), static_cast<int32>(ZOffset))));
}

void ATU_KillhouseGenerator::GenerateWalls(float ZOffset)
{
    const float WallZ = ZOffset + (FloorHeight * 0.5f);
    const float HalfW = BuildingWidth * 0.5f;
    const float HalfL = BuildingLength * 0.5f;
    const float DoorWidth = 180.0f;

    AddCube(FVector(-HalfW, 0.0f, WallZ), FVector(WallThickness * 0.5f, HalfL, FloorHeight * 0.5f), TEXT("Wall_Left"));

    AddCube(FVector(HalfW, -HalfL * 0.45f, WallZ), FVector(WallThickness * 0.5f, HalfL * 0.55f, FloorHeight * 0.5f), TEXT("Wall_Right_A"));
    AddCube(FVector(HalfW, HalfL * 0.45f, WallZ), FVector(WallThickness * 0.5f, HalfL * 0.55f, FloorHeight * 0.5f), TEXT("Wall_Right_B"));

    AddCube(FVector(-HalfW * 0.6f, -HalfL, WallZ), FVector(HalfW * 0.4f, WallThickness * 0.5f, FloorHeight * 0.5f), TEXT("Wall_Front_A"));
    AddCube(FVector(HalfW * 0.6f, -HalfL, WallZ), FVector(HalfW * 0.4f, WallThickness * 0.5f, FloorHeight * 0.5f), TEXT("Wall_Front_B"));

    AddCube(FVector(-HalfW * 0.65f, HalfL, WallZ), FVector(HalfW * 0.35f, WallThickness * 0.5f, FloorHeight * 0.5f), TEXT("Wall_Rear_A"));
    AddCube(FVector(0.0f, HalfL, WallZ), FVector(HalfW * 0.15f - DoorWidth * 0.5f, WallThickness * 0.5f, FloorHeight * 0.5f), TEXT("Wall_Rear_B"));
    AddCube(FVector(HalfW * 0.65f, HalfL, WallZ), FVector(HalfW * 0.35f, WallThickness * 0.5f, FloorHeight * 0.5f), TEXT("Wall_Rear_C"));
}

void ATU_KillhouseGenerator::GenerateRoom(const FVector& Center, const FVector2D& Size, float ZBase, const FString& RoomName, bool bLabel, bool bDoorToHallway, bool bDoorAtPositiveX)
{
    const float WallZ = ZBase + (FloorHeight * 0.5f);
    const float HalfW = Size.X * 0.5f;
    const float HalfL = Size.Y * 0.5f;

    AddCube(Center + FVector(0.0f, -HalfL, WallZ), FVector(HalfW, WallThickness * 0.5f, FloorHeight * 0.5f), *FString::Printf(TEXT("%s_Wall_S"), *RoomName));
    AddCube(Center + FVector(0.0f, HalfL, WallZ), FVector(HalfW, WallThickness * 0.5f, FloorHeight * 0.5f), *FString::Printf(TEXT("%s_Wall_N"), *RoomName));

    const float XSign = bDoorAtPositiveX ? 1.0f : -1.0f;
    const float DoorX = Center.X + (HalfW * XSign);

    if (bDoorToHallway)
    {
        AddCube(FVector(DoorX, Center.Y - HalfL * 0.6f, WallZ), FVector(WallThickness * 0.5f, HalfL * 0.4f, FloorHeight * 0.5f), *FString::Printf(TEXT("%s_DoorWall_A"), *RoomName));
        AddCube(FVector(DoorX, Center.Y + HalfL * 0.6f, WallZ), FVector(WallThickness * 0.5f, HalfL * 0.4f, FloorHeight * 0.5f), *FString::Printf(TEXT("%s_DoorWall_B"), *RoomName));
    }
    else
    {
        AddCube(FVector(DoorX, Center.Y, WallZ), FVector(WallThickness * 0.5f, HalfL, FloorHeight * 0.5f), *FString::Printf(TEXT("%s_Wall_Closed"), *RoomName));
    }

    const float OppX = Center.X - (HalfW * XSign);
    AddCube(FVector(OppX, Center.Y, WallZ), FVector(WallThickness * 0.5f, HalfL, FloorHeight * 0.5f), *FString::Printf(TEXT("%s_Wall_Opp"), *RoomName));

    AddCube(Center + FVector(0.0f, 0.0f, ZBase + FloorHeight - 10.0f), FVector(HalfW, HalfL, 10.0f), *FString::Printf(TEXT("%s_Ceiling"), *RoomName));

    if (bLabel)
    {
        SpawnDebugLabel(RoomName, Center + FVector(0.0f, 0.0f, 120.0f));
    }

    const float CoverSize = 40.0f;
    AddCube(Center + FVector(0.0f, 0.0f, 40.0f), FVector(CoverSize, CoverSize * 1.2f, 40.0f), *FString::Printf(TEXT("%s_Cover"), *RoomName));
}

void ATU_KillhouseGenerator::GenerateStairs(float ZBase)
{
    const int32 StepCount = 8;
    const float StepHeight = FloorHeight / StepCount;
    const float StepDepth = 70.0f;

    FVector Start(BuildingWidth * 0.17f, -BuildingLength * 0.15f, ZBase + (StepHeight * 0.5f));
    for (int32 i = 0; i < StepCount; ++i)
    {
        const FVector StepLoc = Start + FVector(0.0f, i * StepDepth, i * StepHeight);
        AddCube(StepLoc, FVector(StairWidth * 0.5f, StepDepth * 0.5f, StepHeight * 0.5f), *FString::Printf(TEXT("StairStep_%d"), i));
    }
}

void ATU_KillhouseGenerator::GenerateSecondFloor(float ZOffset)
{
    GenerateFloor(ZOffset);
    GenerateWalls(ZOffset);

    const int32 Rooms = FMath::Clamp(SecondFloorRoomCount, 3, 8);
    const float RoomDepth = BuildingLength * 0.28f;
    const float LeftRoomWidth = (BuildingWidth - HallwayWidth) * 0.5f;

    GenerateRoom(FVector(-BuildingWidth * 0.25f, -BuildingLength * 0.28f, ZOffset), FVector2D(LeftRoomWidth, RoomDepth), ZOffset, TEXT("Landing"), false, true, false);
    GenerateRoom(FVector(-BuildingWidth * 0.25f, BuildingLength * 0.08f, ZOffset), FVector2D(LeftRoomWidth, RoomDepth), ZOffset, TEXT("Barracks"), bGenerateDebugLabels, true, false);
    if (Rooms >= 4)
    {
        GenerateRoom(FVector(BuildingWidth * 0.25f, BuildingLength * 0.08f, ZOffset), FVector2D(LeftRoomWidth, RoomDepth), ZOffset, TEXT("Secondary"), false, true, true);
    }
    GenerateRoom(FVector(BuildingWidth * 0.25f, BuildingLength * 0.35f, ZOffset), FVector2D(LeftRoomWidth, RoomDepth), ZOffset, TEXT("Comms"), bGenerateDebugLabels, true, true);

    AddCube(FVector(0.0f, -BuildingLength * 0.25f, ZOffset + 8.0f), FVector(HallwayWidth * 0.5f, 160.0f, 8.0f), TEXT("Catwalk"));

    const float OverlookDepth = 120.0f;
    AddCube(FVector(0.0f, -20.0f, ZOffset + 8.0f), FVector(HallwayWidth * 0.5f, OverlookDepth, 8.0f), TEXT("OverlookBridge"));

    if (bGenerateDebugLabels)
    {
        SpawnDebugLabel(TEXT("Stairwell"), FVector(BuildingWidth * 0.25f, -BuildingLength * 0.2f, 120.0f));
        SpawnDebugLabel(TEXT("Comms"), FVector(BuildingWidth * 0.25f, BuildingLength * 0.35f, ZOffset + 120.0f));
    }
}

void ATU_KillhouseGenerator::SpawnDebugLabel(const FString& Label, const FVector& Location)
{
    UTextRenderComponent* TextComponent = NewObject<UTextRenderComponent>(this);
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
