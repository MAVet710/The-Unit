#include "TU_DonetskArtema60Building.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATU_DonetskArtema60Building::ATU_DonetskArtema60Building()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    CubeMesh = CubeFinder.Object;
    CylinderMesh = CylinderFinder.Object;
}

void ATU_DonetskArtema60Building::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ClearGenerated();
    GeneratedNameCounter = 0;
    RebuildBuilding();
}

void ATU_DonetskArtema60Building::ClearGenerated()
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

UStaticMeshComponent* ATU_DonetskArtema60Building::AddBox(
    const FVector& Location,
    const FVector& Extents,
    const FString& BaseName,
    const FRotator& Rotation)
{
    if (!CubeMesh)
    {
        return nullptr;
    }

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

UStaticMeshComponent* ATU_DonetskArtema60Building::AddCylinder(
    const FVector& Location,
    float RadiusCm,
    float HeightCm,
    const FString& BaseName)
{
    if (!CylinderMesh)
    {
        return nullptr;
    }

    const FName Name(*FString::Printf(TEXT("%s_%04d"), *BaseName, GeneratedNameCounter++));
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, Name);
    Mesh->SetStaticMesh(CylinderMesh);
    Mesh->SetRelativeLocation(Location);
    // Engine cylinder is 100 cm diameter and 100 cm tall.
    Mesh->SetRelativeScale3D(FVector(RadiusCm / 50.0f, RadiusCm / 50.0f, HeightCm / 100.0f));
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Mesh->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Mesh->RegisterComponent();
    GeneratedComponents.Add(Mesh);
    return Mesh;
}

void ATU_DonetskArtema60Building::RebuildBuilding()
{
    if (!CubeMesh)
    {
        return;
    }

    BuildMainMass();
    BuildStreetFacade();
    BuildRoundedProjection();
    BuildStairTower();
    if (bCurrentPostwarConfiguration)
    {
        BuildPostwarDetails();
    }
}

void ATU_DonetskArtema60Building::BuildMainMass()
{
    const int32 Storeys = bCurrentPostwarConfiguration ? 4 : 3;
    const float TotalHeight = GroundFloorHeightCm + UpperFloorHeightCm * (Storeys - 1);

    AddBox(
        FVector(0.0f, 0.0f, TotalHeight * 0.5f),
        FVector(EstimatedFrontageCm * 0.5f, EstimatedDepthCm * 0.5f, TotalHeight * 0.5f),
        TEXT("Artema60_MainMass"));

    // Dark contrasting plinth is a documented visual characteristic of the current facade.
    AddBox(
        FVector(0.0f, -EstimatedDepthCm * 0.5f - 12.0f, 52.0f),
        FVector(EstimatedFrontageCm * 0.5f + 10.0f, 18.0f, 52.0f),
        TEXT("Artema60_StreetPlinth"));
}

void ATU_DonetskArtema60Building::BuildStreetFacade()
{
    const int32 Storeys = bCurrentPostwarConfiguration ? 4 : 3;
    const int32 StraightFacadeBays = 10;
    const float BayWidth = (EstimatedFrontageCm - RoundedProjectionRadiusCm * 2.0f - StairTowerWidthCm) / StraightFacadeBays;
    const float FrontY = -EstimatedDepthCm * 0.5f - 18.0f;

    float FloorBase = 0.0f;
    for (int32 Floor = 0; Floor < Storeys; ++Floor)
    {
        const float FloorHeight = Floor == 0 ? GroundFloorHeightCm : UpperFloorHeightCm;
        const float WindowCenterZ = FloorBase + FloorHeight * (Floor == 0 ? 0.55f : 0.53f);
        const float WindowHeight = FloorHeight * (Floor == 0 ? 0.52f : 0.60f);

        if (bGenerateWindowReferencePlates)
        {
            for (int32 Bay = 0; Bay < StraightFacadeBays; ++Bay)
            {
                const float XStart = -EstimatedFrontageCm * 0.5f + StairTowerWidthCm + BayWidth * 0.5f;
                const float X = XStart + Bay * BayWidth;
                AddBox(
                    FVector(X, FrontY, WindowCenterZ),
                    FVector(FMath::Max(70.0f, BayWidth * 0.25f), 12.0f, WindowHeight * 0.5f),
                    TEXT("Artema60_WindowReference"));
            }
        }

        FloorBase += FloorHeight;

        if (Floor < Storeys - 1)
        {
            // Strong horizontal floor stratification is characteristic of the building's composition.
            AddBox(
                FVector(0.0f, FrontY - 8.0f, FloorBase),
                FVector(EstimatedFrontageCm * 0.5f + 18.0f, 24.0f, 16.0f),
                TEXT("Artema60_FloorBand"));
        }
    }
}

void ATU_DonetskArtema60Building::BuildRoundedProjection()
{
    if (!CylinderMesh)
    {
        return;
    }

    const int32 Storeys = bCurrentPostwarConfiguration ? 4 : 3;
    const float TotalHeight = GroundFloorHeightCm + UpperFloorHeightCm * (Storeys - 1);
    const float ProjectionX = EstimatedFrontageCm * 0.5f - RoundedProjectionRadiusCm - 180.0f;
    const float ProjectionY = -EstimatedDepthCm * 0.5f - RoundedProjectionRadiusCm * 0.46f;

    AddCylinder(
        FVector(ProjectionX, ProjectionY, TotalHeight * 0.5f),
        RoundedProjectionRadiusCm,
        TotalHeight,
        TEXT("Artema60_SemicircularProjection"));

    // Current facade photographs show the rounded projection articulated by tall white columns.
    if (bCurrentPostwarConfiguration)
    {
        constexpr int32 ColumnCount = 5;
        for (int32 Index = 0; Index < ColumnCount; ++Index)
        {
            const float AngleDeg = FMath::Lerp(205.0f, 335.0f, static_cast<float>(Index) / (ColumnCount - 1));
            const float Angle = FMath::DegreesToRadians(AngleDeg);
            const FVector ColumnLocation(
                ProjectionX + FMath::Cos(Angle) * (RoundedProjectionRadiusCm + 28.0f),
                ProjectionY + FMath::Sin(Angle) * (RoundedProjectionRadiusCm + 28.0f),
                730.0f);
            AddCylinder(ColumnLocation, 24.0f, 1080.0f, TEXT("Artema60_ProjectionColumn"));
        }
    }
}

void ATU_DonetskArtema60Building::BuildStairTower()
{
    const int32 Storeys = bCurrentPostwarConfiguration ? 4 : 3;
    const float MainHeight = GroundFloorHeightCm + UpperFloorHeightCm * (Storeys - 1);
    const float TowerHeight = MainHeight + (bCurrentPostwarConfiguration ? 170.0f : 40.0f);
    const float FrontY = -EstimatedDepthCm * 0.5f - 80.0f;
    const float TowerX = -EstimatedFrontageCm * 0.5f + StairTowerWidthCm * 0.5f;

    AddBox(
        FVector(TowerX, FrontY, TowerHeight * 0.5f),
        FVector(StairTowerWidthCm * 0.5f, 260.0f, TowerHeight * 0.5f),
        TEXT("Artema60_StairTower"));

    if (bGenerateWindowReferencePlates)
    {
        // Tall vertical openings are documented on the street facade; these plates are calibration markers, not boolean holes.
        for (int32 Index = 0; Index < (bCurrentPostwarConfiguration ? 3 : 2); ++Index)
        {
            AddBox(
                FVector(TowerX, FrontY - 270.0f, 310.0f + Index * 390.0f),
                FVector(StairTowerWidthCm * 0.20f, 14.0f, 150.0f),
                TEXT("Artema60_TallTowerWindowReference"));
        }
    }
}

void ATU_DonetskArtema60Building::BuildPostwarDetails()
{
    const float TotalHeight = GroundFloorHeightCm + UpperFloorHeightCm * 3.0f;
    const float FrontY = -EstimatedDepthCm * 0.5f - 42.0f;

    // Postwar reconstruction added the fourth storey, decorative parapet and balcony balustrades.
    AddBox(
        FVector(0.0f, FrontY, TotalHeight + 55.0f),
        FVector(EstimatedFrontageCm * 0.5f + 25.0f, 32.0f, 55.0f),
        TEXT("Artema60_DecorativeParapet"));

    const float ProjectionX = EstimatedFrontageCm * 0.5f - RoundedProjectionRadiusCm - 180.0f;
    for (int32 Floor = 1; Floor < 4; ++Floor)
    {
        const float Z = GroundFloorHeightCm + UpperFloorHeightCm * Floor - 40.0f;
        AddBox(
            FVector(ProjectionX, FrontY - RoundedProjectionRadiusCm * 0.72f, Z),
            FVector(RoundedProjectionRadiusCm * 0.92f, 85.0f, 12.0f),
            TEXT("Artema60_BalconySlab"));
        AddBox(
            FVector(ProjectionX, FrontY - RoundedProjectionRadiusCm * 0.91f, Z + 50.0f),
            FVector(RoundedProjectionRadiusCm * 0.92f, 8.0f, 50.0f),
            TEXT("Artema60_BalustradeReference"));
    }
}