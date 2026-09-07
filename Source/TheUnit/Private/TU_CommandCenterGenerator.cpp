#include "TU_CommandCenterGenerator.h"

#include "TU_CommandCenterStation.h"
#include "TU_RangeTarget.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATU_CommandCenterGenerator::ATU_CommandCenterGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeFinder.Object;
}

void ATU_CommandCenterGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ClearGeneratedComponents();
    BuildHub();
}

void ATU_CommandCenterGenerator::BeginPlay()
{
    Super::BeginPlay();

    if (StationSpawns.IsEmpty())
    {
        ClearGeneratedComponents();
        BuildHub();
    }

    if (!bSpawnRuntimeStations || !GetWorld())
    {
        return;
    }

    ClearRuntimeStations();

    for (const FStationSpawn& Spawn : StationSpawns)
    {
        const FVector WorldLocation = GetActorTransform().TransformPosition(Spawn.Location);
        const FRotator WorldRotation = GetActorTransform().TransformRotation(Spawn.Rotation.Quaternion()).Rotator();

        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ATU_CommandCenterStation* Station = GetWorld()->SpawnActor<ATU_CommandCenterStation>(
            ATU_CommandCenterStation::StaticClass(), WorldLocation, WorldRotation, Params);
        if (!Station)
        {
            continue;
        }

        Station->ConfigureStation(
            static_cast<ETUCommandCenterStationType>(Spawn.TypeValue),
            FText::FromString(Spawn.Label),
            Spawn.MissionId);
        RuntimeStations.Add(Station);
    }

    const TArray<FVector> TargetLocations = {
        FVector(1780.0f, -2450.0f, 110.0f),
        FVector(2150.0f, -2450.0f, 110.0f),
        FVector(2520.0f, -2450.0f, 110.0f),
        FVector(2890.0f, -2450.0f, 110.0f)
    };

    for (const FVector& LocalLocation : TargetLocations)
    {
        const FVector WorldLocation = GetActorTransform().TransformPosition(LocalLocation);
        const FRotator WorldRotation = GetActorRotation() + FRotator(0.0f, 180.0f, 0.0f);

        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GetWorld()->SpawnActor<ATU_RangeTarget>(ATU_RangeTarget::StaticClass(), WorldLocation, WorldRotation, Params);
    }
}

void ATU_CommandCenterGenerator::BuildHub()
{
    if (!CubeMesh)
    {
        return;
    }

    StationSpawns.Reset();
    BuildSecureCorridor();
    BuildArmory();
    BuildCage();
    BuildBriefingRoom();
    BuildTestRange();
    BuildOperationsDetails();
}

void ATU_CommandCenterGenerator::BuildSecureCorridor()
{
    const float HalfLength = HubLength * 0.5f;
    const float HalfCorridor = CorridorWidth * 0.5f;
    const float WallThickness = 18.0f;
    const float DoorHalfWidth = 105.0f;

    AddCube(FVector(0.0f, 0.0f, -10.0f), FVector(HalfCorridor, HalfLength, 10.0f), TEXT("CorridorFloor"));
    AddCube(FVector(0.0f, 0.0f, CeilingHeight), FVector(HalfCorridor, HalfLength, 8.0f), TEXT("CorridorDropCeiling"));

    // Build segmented side walls so every destination is physically reachable from the corridor.
    auto AddSegmentedWall = [this, HalfLength, WallThickness, DoorHalfWidth](float X, TArray<float> DoorCenters, const FString& Prefix)
    {
        DoorCenters.Sort();
        float SegmentStart = -HalfLength;
        int32 SegmentIndex = 0;

        for (const float DoorCenter : DoorCenters)
        {
            const float SegmentEnd = DoorCenter - DoorHalfWidth;
            if (SegmentEnd > SegmentStart)
            {
                const float Length = SegmentEnd - SegmentStart;
                AddCube(
                    FVector(X, (SegmentStart + SegmentEnd) * 0.5f, CeilingHeight * 0.5f),
                    FVector(WallThickness * 0.5f, Length * 0.5f, CeilingHeight * 0.5f),
                    *FString::Printf(TEXT("%s_%d"), *Prefix, SegmentIndex++));
            }
            SegmentStart = DoorCenter + DoorHalfWidth;
        }

        if (SegmentStart < HalfLength)
        {
            const float Length = HalfLength - SegmentStart;
            AddCube(
                FVector(X, (SegmentStart + HalfLength) * 0.5f, CeilingHeight * 0.5f),
                FVector(WallThickness * 0.5f, Length * 0.5f, CeilingHeight * 0.5f),
                *FString::Printf(TEXT("%s_%d"), *Prefix, SegmentIndex));
        }
    };

    AddSegmentedWall(-HalfCorridor, { -1750.0f, 650.0f }, TEXT("CorridorWallWest"));
    AddSegmentedWall(HalfCorridor, { -2150.0f, 950.0f }, TEXT("CorridorWallEast"));

    // Drop-ceiling fluorescent-strip placeholders echo the supplied Operator reference.
    for (int32 Index = -6; Index <= 6; ++Index)
    {
        AddCube(
            FVector(0.0f, Index * 520.0f, CeilingHeight - 12.0f),
            FVector(35.0f, 150.0f, 5.0f),
            *FString::Printf(TEXT("CorridorLight_%d"), Index));
    }

    if (bGenerateLabels)
    {
        AddLabel(
            TEXT("THE UNIT // SPECIAL OPERATIONS COMMAND"),
            FVector(0.0f, -HalfLength + 180.0f, 210.0f),
            FRotator::ZeroRotator);
    }
}

void ATU_CommandCenterGenerator::BuildArmory()
{
    const FVector Center(-1350.0f, -1750.0f, 0.0f);
    const FVector RoomHalf(1000.0f, 1050.0f, CeilingHeight * 0.5f);
    const float Wall = 18.0f;

    AddCube(Center + FVector(0.0f, 0.0f, -10.0f), FVector(RoomHalf.X, RoomHalf.Y, 10.0f), TEXT("ArmoryFloor"));
    AddCube(FVector(-265.0f, -1750.0f, -10.0f), FVector(85.0f, 105.0f, 10.0f), TEXT("ArmoryThreshold"));
    AddCube(Center + FVector(0.0f, 0.0f, CeilingHeight), FVector(RoomHalf.X, RoomHalf.Y, 8.0f), TEXT("ArmoryCeiling"));
    AddCube(Center + FVector(-RoomHalf.X, 0.0f, CeilingHeight * 0.5f), FVector(Wall * 0.5f, RoomHalf.Y, RoomHalf.Z), TEXT("ArmoryWallWest"));
    AddCube(Center + FVector(0.0f, -RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("ArmoryWallSouth"));
    AddCube(Center + FVector(0.0f, RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("ArmoryWallNorth"));

    AddCube(Center + FVector(-RoomHalf.X + 35.0f, 80.0f, 165.0f), FVector(20.0f, 720.0f, 125.0f), TEXT("WeaponWallBacker"));
    for (int32 Row = 0; Row < 4; ++Row)
    {
        for (int32 Column = 0; Column < 3; ++Column)
        {
            const float Y = -470.0f + Column * 430.0f;
            const float Z = 95.0f + Row * 55.0f;
            AddCube(
                Center + FVector(-RoomHalf.X + 65.0f, Y, Z),
                FVector(18.0f, 150.0f, 9.0f),
                *FString::Printf(TEXT("WeaponDisplay_%d_%d"), Row, Column));
        }
    }

    AddCube(Center + FVector(260.0f, -360.0f, 42.0f), FVector(260.0f, 95.0f, 42.0f), TEXT("WeaponBenchA"));
    AddCube(Center + FVector(260.0f, 360.0f, 42.0f), FVector(260.0f, 95.0f, 42.0f), TEXT("WeaponBenchB"));

    AddStationMarker(0, TEXT("ARMORY // SELECT WEAPONS"), Center + FVector(360.0f, 0.0f, 65.0f), FRotator(0.0f, 180.0f, 0.0f));
    AddStationMarker(1, TEXT("WEAPON CUSTOMIZATION"), Center + FVector(250.0f, -360.0f, 95.0f), FRotator(0.0f, 180.0f, 0.0f));

    if (bGenerateLabels)
    {
        AddLabel(TEXT("ARMORY"), Center + FVector(-930.0f, 0.0f, 285.0f));
        AddLabel(TEXT("WEAPON CUSTOMIZATION"), Center + FVector(250.0f, -360.0f, 115.0f), FRotator(0.0f, 180.0f, 0.0f));
    }
}

void ATU_CommandCenterGenerator::BuildCage()
{
    const FVector Center(-1400.0f, 650.0f, 0.0f);
    const FVector RoomHalf(950.0f, 900.0f, CeilingHeight * 0.5f);
    const float Wall = 18.0f;

    AddCube(Center + FVector(0.0f, 0.0f, -10.0f), FVector(RoomHalf.X, RoomHalf.Y, 10.0f), TEXT("CageFloor"));
    AddCube(FVector(-315.0f, 650.0f, -10.0f), FVector(135.0f, 105.0f, 10.0f), TEXT("CageThreshold"));
    AddCube(Center + FVector(0.0f, 0.0f, CeilingHeight), FVector(RoomHalf.X, RoomHalf.Y, 8.0f), TEXT("CageCeiling"));
    AddCube(Center + FVector(-RoomHalf.X, 0.0f, CeilingHeight * 0.5f), FVector(Wall * 0.5f, RoomHalf.Y, RoomHalf.Z), TEXT("CageWallWest"));
    AddCube(Center + FVector(0.0f, RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("CageWallNorth"));
    AddCube(Center + FVector(0.0f, -RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("CageWallSouth"));

    for (int32 Bay = 0; Bay < 3; ++Bay)
    {
        const float YBase = -560.0f + Bay * 560.0f;
        AddCube(Center + FVector(-760.0f, YBase, 140.0f), FVector(12.0f, 240.0f, 140.0f), *FString::Printf(TEXT("CageBayBack_%d"), Bay));
        for (int32 Bar = -4; Bar <= 4; ++Bar)
        {
            AddCube(Center + FVector(-560.0f, YBase + Bar * 48.0f, 140.0f), FVector(8.0f, 5.0f, 140.0f), *FString::Printf(TEXT("CageBar_%d_%d"), Bay, Bar));
        }
        AddCube(Center + FVector(-560.0f, YBase, 278.0f), FVector(8.0f, 240.0f, 6.0f), *FString::Printf(TEXT("CageTop_%d"), Bay));
    }

    AddCube(Center + FVector(280.0f, 250.0f, 43.0f), FVector(260.0f, 100.0f, 43.0f), TEXT("GearBench"));
    for (int32 Locker = 0; Locker < 5; ++Locker)
    {
        AddCube(
            Center + FVector(620.0f, -520.0f + Locker * 220.0f, 125.0f),
            FVector(90.0f, 90.0f, 125.0f),
            *FString::Printf(TEXT("GearLocker_%d"), Locker));
    }

    AddStationMarker(2, TEXT("CAGE // EQUIPMENT + UNIFORMS"), Center + FVector(180.0f, -150.0f, 65.0f), FRotator(0.0f, 180.0f, 0.0f));
    AddStationMarker(3, TEXT("GEAR / UNIFORM BENCH"), Center + FVector(280.0f, 250.0f, 95.0f), FRotator(0.0f, 180.0f, 0.0f));

    if (bGenerateLabels)
    {
        AddLabel(TEXT("THE CAGE // EQUIPMENT + UNIFORMS"), Center + FVector(-900.0f, 0.0f, 285.0f));
    }
}

void ATU_CommandCenterGenerator::BuildBriefingRoom()
{
    const FVector Center(1400.0f, 950.0f, 0.0f);
    const FVector RoomHalf(1000.0f, 1050.0f, CeilingHeight * 0.5f);
    const float Wall = 18.0f;

    AddCube(Center + FVector(0.0f, 0.0f, -10.0f), FVector(RoomHalf.X, RoomHalf.Y, 10.0f), TEXT("BriefingFloor"));
    AddCube(FVector(290.0f, 950.0f, -10.0f), FVector(110.0f, 105.0f, 10.0f), TEXT("BriefingThreshold"));
    AddCube(Center + FVector(0.0f, 0.0f, CeilingHeight), FVector(RoomHalf.X, RoomHalf.Y, 8.0f), TEXT("BriefingCeiling"));
    AddCube(Center + FVector(RoomHalf.X, 0.0f, CeilingHeight * 0.5f), FVector(Wall * 0.5f, RoomHalf.Y, RoomHalf.Z), TEXT("BriefingWallEast"));
    AddCube(Center + FVector(0.0f, RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("BriefingWallNorth"));
    AddCube(Center + FVector(0.0f, -RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("BriefingWallSouth"));

    AddCube(Center + FVector(50.0f, 0.0f, 45.0f), FVector(430.0f, 160.0f, 18.0f), TEXT("BriefingTable"));
    for (int32 Side = -1; Side <= 1; Side += 2)
    {
        for (int32 Seat = -2; Seat <= 2; ++Seat)
        {
            AddCube(
                Center + FVector(Seat * 170.0f, Side * 285.0f, 45.0f),
                FVector(35.0f, 35.0f, 45.0f),
                *FString::Printf(TEXT("BriefingChair_%d_%d"), Side, Seat));
        }
    }

    AddCube(Center + FVector(780.0f, 0.0f, 190.0f), FVector(14.0f, 380.0f, 95.0f), TEXT("BriefingWallDisplay"));

    // Physical secure laptop is the mission UI anchor, matching the user's Operator reference.
    AddCube(Center + FVector(-170.0f, 0.0f, 76.0f), FVector(38.0f, 52.0f, 4.0f), TEXT("BriefingLaptopBase"));
    AddCube(Center + FVector(-205.0f, 0.0f, 113.0f), FVector(4.0f, 52.0f, 38.0f), TEXT("BriefingLaptopScreen"), FRotator(0.0f, 0.0f, -12.0f));

    AddStationMarker(5, TEXT("CLASSIFIED OPERATIONS TERMINAL"), Center + FVector(-150.0f, -40.0f, 100.0f), FRotator::ZeroRotator, TEXT("OP_KILLHOUSE"));
    AddStationMarker(6, TEXT("COMMIT LOADOUT / DEPLOY"), Center + FVector(650.0f, -650.0f, 70.0f), FRotator(0.0f, 180.0f, 0.0f), TEXT("OP_KILLHOUSE"));

    if (bGenerateLabels)
    {
        AddLabel(TEXT("BRIEFING // OPERATIONS"), Center + FVector(930.0f, 0.0f, 285.0f), FRotator(0.0f, 180.0f, 0.0f));
    }
}

void ATU_CommandCenterGenerator::BuildTestRange()
{
    const FVector Center(1850.0f, -2150.0f, 0.0f);
    const FVector RoomHalf(1450.0f, 700.0f, CeilingHeight * 0.5f);
    const float Wall = 18.0f;

    AddCube(Center + FVector(0.0f, 0.0f, -10.0f), FVector(RoomHalf.X, RoomHalf.Y, 10.0f), TEXT("RangeFloor"));
    AddCube(FVector(290.0f, -2150.0f, -10.0f), FVector(110.0f, 105.0f, 10.0f), TEXT("RangeThreshold"));
    AddCube(Center + FVector(0.0f, 0.0f, CeilingHeight), FVector(RoomHalf.X, RoomHalf.Y, 8.0f), TEXT("RangeCeiling"));
    AddCube(Center + FVector(0.0f, -RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("RangeWallSouth"));
    AddCube(Center + FVector(0.0f, RoomHalf.Y, CeilingHeight * 0.5f), FVector(RoomHalf.X, Wall * 0.5f, RoomHalf.Z), TEXT("RangeWallNorth"));
    AddCube(Center + FVector(RoomHalf.X, 0.0f, CeilingHeight * 0.5f), FVector(Wall * 0.5f, RoomHalf.Y, RoomHalf.Z), TEXT("RangeBackstop"));

    AddCube(Center + FVector(-900.0f, 0.0f, 4.0f), FVector(18.0f, RoomHalf.Y, 4.0f), TEXT("RangeFiringLine"));
    for (int32 Lane = -2; Lane <= 2; ++Lane)
    {
        AddCube(
            Center + FVector(-820.0f, Lane * 240.0f, 80.0f),
            FVector(140.0f, 6.0f, 80.0f),
            *FString::Printf(TEXT("RangeDivider_%d"), Lane));
    }

    AddStationMarker(4, TEXT("LIVE FIRE // TEST SELECTED WEAPON"), Center + FVector(-1050.0f, 0.0f, 65.0f), FRotator::ZeroRotator);

    if (bGenerateLabels)
    {
        AddLabel(TEXT("TEST FIRE RANGE"), Center + FVector(-1180.0f, 0.0f, 285.0f), FRotator::ZeroRotator);
    }
}

void ATU_CommandCenterGenerator::BuildOperationsDetails()
{
    // Visible door leaves sit beside the openings rather than blocking the corridor.
    struct FDoorVisual
    {
        FVector Location;
        float Yaw;
        FString Name;
    };

    const TArray<FDoorVisual> Doors = {
        { FVector(-195.0f, -1855.0f, 105.0f), 0.0f, TEXT("ArmoryDoor") },
        { FVector(-195.0f, 545.0f, 105.0f), 0.0f, TEXT("CageDoor") },
        { FVector(195.0f, 845.0f, 105.0f), 0.0f, TEXT("BriefingDoor") },
        { FVector(195.0f, -2255.0f, 105.0f), 0.0f, TEXT("RangeDoor") }
    };

    for (const FDoorVisual& Door : Doors)
    {
        AddCube(Door.Location, FVector(8.0f, 55.0f, 105.0f), *Door.Name, FRotator(0.0f, Door.Yaw, 0.0f));
    }

    if (bGenerateLabels)
    {
        AddLabel(TEXT("ARMORY"), FVector(-190.0f, -1750.0f, 205.0f), FRotator(0.0f, 90.0f, 0.0f));
        AddLabel(TEXT("CAGE"), FVector(-190.0f, 650.0f, 205.0f), FRotator(0.0f, 90.0f, 0.0f));
        AddLabel(TEXT("BRIEFING"), FVector(190.0f, 950.0f, 205.0f), FRotator(0.0f, -90.0f, 0.0f));
        AddLabel(TEXT("RANGE"), FVector(190.0f, -2150.0f, 205.0f), FRotator(0.0f, -90.0f, 0.0f));
    }
}

UStaticMeshComponent* ATU_CommandCenterGenerator::AddCube(const FVector& Location, const FVector& Extents, const FName& Name, const FRotator& Rotation)
{
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

UArrowComponent* ATU_CommandCenterGenerator::AddMarker(const FVector& Location, const FRotator& Rotation, const FName& Name)
{
    UArrowComponent* Marker = NewObject<UArrowComponent>(this, Name);
    Marker->SetRelativeLocation(Location);
    Marker->SetRelativeRotation(Rotation);
    Marker->ArrowSize = 1.25f;
    Marker->SetHiddenInGame(true);
    Marker->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Marker->RegisterComponent();
    GeneratedComponents.Add(Marker);
    return Marker;
}

void ATU_CommandCenterGenerator::AddLabel(const FString& Text, const FVector& Location, const FRotator& Rotation)
{
    UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
    Label->SetText(FText::FromString(Text));
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetWorldSize(34.0f);
    Label->SetRelativeLocation(Location);
    Label->SetRelativeRotation(Rotation);
    Label->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Label->RegisterComponent();
    GeneratedComponents.Add(Label);
}

void ATU_CommandCenterGenerator::AddStationMarker(uint8 TypeValue, const FString& Label, const FVector& Location, const FRotator& Rotation, FName MissionId)
{
    FStationSpawn Spawn;
    Spawn.Location = Location;
    Spawn.Rotation = Rotation;
    Spawn.TypeValue = TypeValue;
    Spawn.Label = Label;
    Spawn.MissionId = MissionId;
    StationSpawns.Add(Spawn);

    AddMarker(Location, Rotation, *FString::Printf(TEXT("Station_%d"), StationSpawns.Num()));
}

void ATU_CommandCenterGenerator::ClearGeneratedComponents()
{
    for (UActorComponent* Component : GeneratedComponents)
    {
        if (IsValid(Component))
        {
            Component->DestroyComponent();
        }
    }
    GeneratedComponents.Reset();
    StationSpawns.Reset();
}

void ATU_CommandCenterGenerator::ClearRuntimeStations()
{
    for (ATU_CommandCenterStation* Station : RuntimeStations)
    {
        if (IsValid(Station))
        {
            Station->Destroy();
        }
    }
    RuntimeStations.Reset();
}
