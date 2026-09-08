#include "TU_HideoutCommandCenterDecorator.h"

#include "TUHideoutProgressionComponent.h"
#include "TU_CommandCenterGenerator.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ATU_HideoutCommandCenterDecorator::ATU_HideoutCommandCenterDecorator()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Progression = CreateDefaultSubobject<UTUHideoutProgressionComponent>(TEXT("HideoutProgression"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeFinder.Object;
}

void ATU_HideoutCommandCenterDecorator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Rebuild();
}

void ATU_HideoutCommandCenterDecorator::BeginPlay()
{
    Super::BeginPlay();

    if (bSnapToCommandCenterAtBeginPlay && GetWorld())
    {
        for (TActorIterator<ATU_CommandCenterGenerator> It(GetWorld()); It; ++It)
        {
            SetActorTransform(It->GetActorTransform());
            break;
        }
    }

    Rebuild();
}

void ATU_HideoutCommandCenterDecorator::ClearGenerated()
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

UStaticMeshComponent* ATU_HideoutCommandCenterDecorator::AddCube(const FName& Name, const FVector& Location, const FVector& Extents, const FRotator& Rotation, bool bCollision)
{
    if (!CubeMesh)
    {
        return nullptr;
    }

    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
    if (!Component)
    {
        return nullptr;
    }

    AddInstanceComponent(Component);
    Component->SetupAttachment(Root);
    Component->SetStaticMesh(CubeMesh);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeRotation(Rotation);
    Component->SetRelativeScale3D(Extents / 50.0f);
    Component->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    Component->RegisterComponent();
    GeneratedComponents.Add(Component);
    return Component;
}

void ATU_HideoutCommandCenterDecorator::Rebuild()
{
    ClearGenerated();
    BuildUtilities();
    BuildStorageAndStaging();
    BuildMaintenance();
    BuildMedical();
    BuildCommsAndPlanning();
    BuildArmoryAndRangeSupport();
}

void ATU_HideoutCommandCenterDecorator::BuildUtilities()
{
    const int32 PowerLevel = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::Power) : 0;
    if (PowerLevel <= 0)
    {
        return;
    }

    AddCube(TEXT("GeneratorBase"), FVector(1750.0f, 2850.0f, 55.0f), FVector(180.0f, 95.0f, 55.0f), FRotator::ZeroRotator, true);
    AddCube(TEXT("GeneratorTop"), FVector(1750.0f, 2850.0f, 130.0f), FVector(135.0f, 75.0f, 20.0f));
    AddCube(TEXT("BreakerPanel"), FVector(2240.0f, 2700.0f, 145.0f), FVector(20.0f, 95.0f, 120.0f));

    for (int32 Index = 0; Index < 6 + PowerLevel * 2; ++Index)
    {
        AddCube(*FString::Printf(TEXT("UtilityConduit_%d"), Index), FVector(-180.0f + Index * 55.0f, 2900.0f, 292.0f), FVector(22.0f, 220.0f, 6.0f));
    }

    if (PowerLevel >= 2)
    {
        AddCube(TEXT("BatteryRack"), FVector(2050.0f, 3050.0f, 100.0f), FVector(150.0f, 65.0f, 100.0f), FRotator::ZeroRotator, true);
    }
    if (PowerLevel >= 3)
    {
        AddCube(TEXT("PowerControlCabinet"), FVector(2350.0f, 3050.0f, 130.0f), FVector(90.0f, 75.0f, 130.0f), FRotator::ZeroRotator, true);
    }
}

void ATU_HideoutCommandCenterDecorator::BuildStorageAndStaging()
{
    const int32 StorageLevel = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::Storage) : 0;
    if (StorageLevel <= 0)
    {
        return;
    }

    for (int32 Rack = 0; Rack < 3 + StorageLevel; ++Rack)
    {
        const float Y = 1850.0f + Rack * 260.0f;
        AddCube(*FString::Printf(TEXT("StorageRack_%d"), Rack), FVector(-2250.0f, Y, 120.0f), FVector(80.0f, 95.0f, 120.0f), FRotator::ZeroRotator, true);
        AddCube(*FString::Printf(TEXT("StorageCrate_%d"), Rack), FVector(-2050.0f, Y, 42.0f), FVector(95.0f, 80.0f, 42.0f), FRotator::ZeroRotator, true);
    }

    AddCube(TEXT("OperatorBench"), FVector(-650.0f, 2750.0f, 38.0f), FVector(260.0f, 70.0f, 38.0f), FRotator::ZeroRotator, true);
    AddCube(TEXT("ReadyRack"), FVector(-1050.0f, 2750.0f, 135.0f), FVector(35.0f, 300.0f, 135.0f));

    if (StorageLevel >= 2)
    {
        AddCube(TEXT("BulkCases"), FVector(-1500.0f, 3150.0f, 55.0f), FVector(260.0f, 140.0f, 55.0f), FRotator::ZeroRotator, true);
    }
}

void ATU_HideoutCommandCenterDecorator::BuildMaintenance()
{
    const int32 Level = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::GearMaintenance) : 0;
    if (Level <= 0)
    {
        return;
    }

    AddCube(TEXT("MaintenanceBench"), FVector(-1550.0f, -2950.0f, 45.0f), FVector(300.0f, 90.0f, 45.0f), FRotator::ZeroRotator, true);
    AddCube(TEXT("MaintenanceBackboard"), FVector(-1550.0f, -3040.0f, 155.0f), FVector(300.0f, 12.0f, 110.0f));

    for (int32 Bin = 0; Bin < 4 + Level * 2; ++Bin)
    {
        AddCube(*FString::Printf(TEXT("PartsBin_%d"), Bin), FVector(-1790.0f + Bin * 80.0f, -3005.0f, 205.0f), FVector(30.0f, 24.0f, 22.0f));
    }

    if (Level >= 2)
    {
        AddCube(TEXT("CleaningStation"), FVector(-950.0f, -2940.0f, 45.0f), FVector(210.0f, 85.0f, 45.0f), FRotator::ZeroRotator, true);
    }
}

void ATU_HideoutCommandCenterDecorator::BuildMedical()
{
    const int32 Level = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::Medical) : 0;
    if (Level <= 0)
    {
        AddCube(TEXT("MedicalUnbuiltCrates"), FVector(2150.0f, 1650.0f, 45.0f), FVector(150.0f, 120.0f, 45.0f), FRotator::ZeroRotator, true);
        return;
    }

    AddCube(TEXT("MedicalCot"), FVector(1900.0f, 1850.0f, 35.0f), FVector(230.0f, 75.0f, 35.0f), FRotator::ZeroRotator, true);
    AddCube(TEXT("MedicalCabinet"), FVector(2260.0f, 1850.0f, 120.0f), FVector(70.0f, 70.0f, 120.0f), FRotator::ZeroRotator, true);

    if (Level >= 2)
    {
        AddCube(TEXT("MedicalSupplyRack"), FVector(2250.0f, 2150.0f, 110.0f), FVector(85.0f, 120.0f, 110.0f), FRotator::ZeroRotator, true);
    }
    if (Level >= 3)
    {
        AddCube(TEXT("MedicalWorkSurface"), FVector(1860.0f, 2200.0f, 42.0f), FVector(220.0f, 75.0f, 42.0f), FRotator::ZeroRotator, true);
    }
}

void ATU_HideoutCommandCenterDecorator::BuildCommsAndPlanning()
{
    const int32 CommsLevel = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::Communications) : 0;
    const int32 PlanningLevel = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::Planning) : 0;

    if (CommsLevel > 0)
    {
        AddCube(TEXT("CommsRack"), FVector(2300.0f, 200.0f, 125.0f), FVector(85.0f, 85.0f, 125.0f), FRotator::ZeroRotator, true);
        AddCube(TEXT("RadioDesk"), FVector(2000.0f, 250.0f, 42.0f), FVector(210.0f, 85.0f, 42.0f), FRotator::ZeroRotator, true);

        for (int32 Unit = 0; Unit < 2 + CommsLevel; ++Unit)
        {
            AddCube(*FString::Printf(TEXT("RadioUnit_%d"), Unit), FVector(2300.0f, 175.0f, 70.0f + Unit * 55.0f), FVector(72.0f, 18.0f, 18.0f));
        }
    }

    if (PlanningLevel > 0)
    {
        AddCube(TEXT("PlanningPinboard"), FVector(2300.0f, 850.0f, 175.0f), FVector(18.0f, 270.0f, 115.0f));
        AddCube(TEXT("PlanningTableSupport"), FVector(1600.0f, 900.0f, 38.0f), FVector(280.0f, 160.0f, 38.0f), FRotator::ZeroRotator, true);

        if (PlanningLevel >= 2)
        {
            AddCube(TEXT("MissionArchiveCabinet"), FVector(2250.0f, 1100.0f, 110.0f), FVector(80.0f, 90.0f, 110.0f), FRotator::ZeroRotator, true);
        }
    }
}

void ATU_HideoutCommandCenterDecorator::BuildArmoryAndRangeSupport()
{
    const int32 ArmoryLevel = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::ArmorySupport) : 0;
    const int32 RangeLevel = Progression ? Progression->GetModuleLevel(ETUHideoutModuleType::RangeSupport) : 0;

    if (ArmoryLevel > 0)
    {
        AddCube(TEXT("ArmoryAmmoCase"), FVector(-2150.0f, -2450.0f, 40.0f), FVector(120.0f, 85.0f, 40.0f), FRotator::ZeroRotator, true);
        AddCube(TEXT("ArmoryAccessoryShelf"), FVector(-2225.0f, -1250.0f, 145.0f), FVector(55.0f, 260.0f, 105.0f));
        if (ArmoryLevel >= 2)
        {
            AddCube(TEXT("ArmoryOpticsCabinet"), FVector(-1950.0f, -1250.0f, 110.0f), FVector(85.0f, 95.0f, 110.0f), FRotator::ZeroRotator, true);
        }
        if (ArmoryLevel >= 3)
        {
            AddCube(TEXT("ArmoryInspectionBench"), FVector(-1050.0f, -2450.0f, 42.0f), FVector(220.0f, 85.0f, 42.0f), FRotator::ZeroRotator, true);
        }
    }

    if (RangeLevel > 0)
    {
        AddCube(TEXT("RangeAmmoBench"), FVector(1150.0f, -2950.0f, 42.0f), FVector(230.0f, 80.0f, 42.0f), FRotator::ZeroRotator, true);
        AddCube(TEXT("RangeTargetStorage"), FVector(950.0f, -3150.0f, 105.0f), FVector(75.0f, 120.0f, 105.0f), FRotator::ZeroRotator, true);
        if (RangeLevel >= 2)
        {
            AddCube(TEXT("RangeMaintenanceCart"), FVector(1450.0f, -3150.0f, 55.0f), FVector(100.0f, 65.0f, 55.0f), FRotator::ZeroRotator, true);
        }
        if (RangeLevel >= 3)
        {
            AddCube(TEXT("RangeChronoStation"), FVector(1750.0f, -3050.0f, 70.0f), FVector(65.0f, 65.0f, 70.0f), FRotator::ZeroRotator, true);
        }
    }
}
