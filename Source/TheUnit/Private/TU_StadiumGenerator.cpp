#include "TU_StadiumGenerator.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATU_StadiumGenerator::ATU_StadiumGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeMeshFinder.Object;
}

void ATU_StadiumGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ClearGeneratedComponents();
    BuildStadium();
}

void ATU_StadiumGenerator::ClearGeneratedComponents()
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

float ATU_StadiumGenerator::FeetToUU(float Feet) const
{
    return Feet * UnitsPerFoot * FMath::Max(FieldScale, 0.1f);
}

FVector ATU_StadiumGenerator::PolarToField(float DistanceFeet, float AngleDegrees) const
{
    const float Distance = FeetToUU(DistanceFeet);
    const float Radians = FMath::DegreesToRadians(AngleDegrees);
    return FVector(FMath::Sin(Radians) * Distance, FMath::Cos(Radians) * Distance, 0.0f);
}

UStaticMeshComponent* ATU_StadiumGenerator::AddCube(const FVector& Location, const FVector& Extents, const FName& Name, const FRotator& Rotation)
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

UArrowComponent* ATU_StadiumGenerator::AddMarker(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, const FName& Name)
{
    UArrowComponent* Marker = NewObject<UArrowComponent>(this, Name);
    Marker->ArrowColor = Color.ToFColor(true);
    Marker->ArrowSize = 1.5f;
    Marker->SetRelativeLocation(Location);
    Marker->SetRelativeRotation(Rotation);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetGenerateOverlapEvents(false);
    Marker->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    Marker->RegisterComponent();

    GeneratedComponents.Add(Marker);
    return Marker;
}

void ATU_StadiumGenerator::SpawnDebugLabel(const FString& Label, const FVector& Location)
{
    UTextRenderComponent* TextComponent = NewObject<UTextRenderComponent>(this);
    TextComponent->SetText(FText::FromString(Label));
    TextComponent->SetHorizontalAlignment(EHTA_Center);
    TextComponent->SetWorldSize(90.0f);
    TextComponent->SetTextRenderColor(FColor::White);
    TextComponent->SetRelativeLocation(Location);
    TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    TextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TextComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    TextComponent->RegisterComponent();

    GeneratedComponents.Add(TextComponent);
}

void ATU_StadiumGenerator::BuildStadium()
{
    if (!CubeMesh)
    {
        return;
    }

    if (bGenerateField)
    {
        GenerateField();
    }

    if (bGenerateInfield)
    {
        GenerateInfield();
        GenerateFoulLines();
    }

    if (bGenerateOutfieldWalls)
    {
        GenerateOutfieldWalls();
    }

    if (bGenerateMonsterWall)
    {
        GenerateMonsterWall();
    }

    if (bGenerateDugouts)
    {
        GenerateDugouts();
    }

    if (bGenerateLowerBowl)
    {
        GenerateLowerBowl();
    }

    if (bGenerateUpperDeck)
    {
        GenerateUpperDeck();
    }

    if (bGenerateExteriorShell)
    {
        GenerateExteriorShell();
    }

    if (bGenerateGameplayMarkers)
    {
        GenerateGameplayMarkers();
    }

    if (bGenerateDebugLabels)
    {
        SpawnDebugLabel(TEXT("Home Plate"), FVector(0.0f, 0.0f, 140.0f));
        SpawnDebugLabel(TEXT("First Base"), FVector(FeetToUU(90.0f), FeetToUU(90.0f), 140.0f));
        SpawnDebugLabel(TEXT("Third Base"), FVector(-FeetToUU(90.0f), FeetToUU(90.0f), 140.0f));
        SpawnDebugLabel(TEXT("Pitcher Mound"), FVector(0.0f, FeetToUU(60.5f), 140.0f));
        SpawnDebugLabel(TEXT("Monster Wall"), PolarToField(LeftFieldDistanceFt, -45.0f) + FVector(0.0f, 0.0f, FeetToUU(10.0f)));
        SpawnDebugLabel(TEXT("Deep Center Triangle"), PolarToField(DeepCenterDistanceFt, 0.0f) + FVector(0.0f, 0.0f, FeetToUU(8.0f)));
        SpawnDebugLabel(TEXT("Right Field"), PolarToField(RightFieldDistanceFt, 45.0f) + FVector(0.0f, 0.0f, FeetToUU(6.0f)));
        SpawnDebugLabel(TEXT("Dugout A"), FVector(-FeetToUU(55.0f), FeetToUU(85.0f), FeetToUU(4.0f)));
        SpawnDebugLabel(TEXT("Dugout B"), FVector(FeetToUU(55.0f), FeetToUU(85.0f), FeetToUU(4.0f)));
        SpawnDebugLabel(TEXT("Press Box"), FVector(0.0f, -FeetToUU(260.0f), FeetToUU(45.0f)));
        SpawnDebugLabel(TEXT("Extraction"), FVector(FeetToUU(330.0f), FeetToUU(170.0f), FeetToUU(5.0f)));
    }
}

void ATU_StadiumGenerator::GenerateField()
{
    const float HalfSpan = FeetToUU(470.0f);
    AddCube(FVector(0.0f, FeetToUU(160.0f), -10.0f), FVector(HalfSpan, HalfSpan, 10.0f), TEXT("FieldBase"));
    AddCube(FVector(0.0f, FeetToUU(200.0f), -6.0f), FVector(FeetToUU(430.0f), FeetToUU(350.0f), 4.0f), TEXT("OutfieldGrass"));
    AddCube(FVector(0.0f, FeetToUU(230.0f), -4.0f), FVector(FeetToUU(450.0f), FeetToUU(380.0f), 2.0f), TEXT("WarningTrack"));
}

void ATU_StadiumGenerator::GenerateInfield()
{
    AddCube(FVector(0.0f, FeetToUU(90.0f), -3.0f), FVector(FeetToUU(65.0f), FeetToUU(65.0f), 3.0f), TEXT("InfieldDiamond"), FRotator(0.0f, 45.0f, 0.0f));
    AddCube(FVector(0.0f, FeetToUU(60.5f), 6.0f), FVector(FeetToUU(8.0f), FeetToUU(8.0f), 6.0f), TEXT("PitcherMound"));

    AddCube(FVector(0.0f, 0.0f, 2.0f), FVector(FeetToUU(2.0f), FeetToUU(2.0f), 2.0f), TEXT("HomePlateMarker"));
    AddCube(FVector(FeetToUU(90.0f), FeetToUU(90.0f), 2.0f), FVector(FeetToUU(2.0f), FeetToUU(2.0f), 2.0f), TEXT("FirstBaseMarker"));
    AddCube(FVector(-FeetToUU(90.0f), FeetToUU(90.0f), 2.0f), FVector(FeetToUU(2.0f), FeetToUU(2.0f), 2.0f), TEXT("ThirdBaseMarker"));
    AddCube(FVector(0.0f, FeetToUU(180.0f), 2.0f), FVector(FeetToUU(2.0f), FeetToUU(2.0f), 2.0f), TEXT("SecondBaseMarker"));
}

void ATU_StadiumGenerator::GenerateFoulLines()
{
    const float Length = FeetToUU(430.0f);
    AddCube(FVector(-Length * 0.5f, Length * 0.5f, 1.5f), FVector(20.0f, Length * 0.5f, 1.5f), TEXT("ThirdBaseFoulLine"), FRotator(0.0f, 45.0f, 0.0f));
    AddCube(FVector(Length * 0.5f, Length * 0.5f, 1.5f), FVector(20.0f, Length * 0.5f, 1.5f), TEXT("FirstBaseFoulLine"), FRotator(0.0f, -45.0f, 0.0f));
}

void ATU_StadiumGenerator::GenerateOutfieldWalls()
{
    const FVector LeftLine = PolarToField(LeftFieldDistanceFt, -45.0f);
    const FVector LeftCenter = PolarToField(LeftCenterDistanceFt, -24.0f);
    const FVector Center = PolarToField(CenterFieldDistanceFt, -6.0f);
    const FVector DeepCenter = PolarToField(DeepCenterDistanceFt, 8.0f);
    const FVector DeepRight = PolarToField(DeepRightDistanceFt, 26.0f);
    const FVector RightLine = PolarToField(RightFieldDistanceFt, 45.0f);

    AddCube((LeftCenter + Center) * 0.5f, FVector((Center - LeftCenter).Size() * 0.5f, 100.0f, FeetToUU(CenterWallHeightFt) * 0.5f), TEXT("Wall_LeftCenter"), (Center - LeftCenter).Rotation());
    AddCube((Center + DeepCenter) * 0.5f, FVector((DeepCenter - Center).Size() * 0.5f, 100.0f, FeetToUU(CenterWallHeightFt) * 0.5f), TEXT("Wall_CenterA"), (DeepCenter - Center).Rotation());
    AddCube((DeepCenter + DeepRight) * 0.5f, FVector((DeepRight - DeepCenter).Size() * 0.5f, 100.0f, FeetToUU(CenterWallHeightFt) * 0.5f), TEXT("Wall_CenterB"), (DeepRight - DeepCenter).Rotation());
    AddCube((DeepRight + RightLine) * 0.5f, FVector((RightLine - DeepRight).Size() * 0.5f, 100.0f, FeetToUU(RightFieldWallHeightFt) * 0.5f), TEXT("Wall_Right"), (RightLine - DeepRight).Rotation());

    AddCube((LeftLine + LeftCenter) * 0.5f, FVector((LeftCenter - LeftLine).Size() * 0.5f, 120.0f, FeetToUU(MonsterWallHeightFt) * 0.5f), TEXT("Wall_LeftLine"), (LeftCenter - LeftLine).Rotation());
}

void ATU_StadiumGenerator::GenerateMonsterWall()
{
    const FVector MonsterCenter = PolarToField(LeftFieldDistanceFt - 6.0f, -42.0f);
    const float Height = FeetToUU(MonsterWallHeightFt);
    AddCube(MonsterCenter + FVector(0.0f, 0.0f, Height * 0.5f), FVector(FeetToUU(52.0f), FeetToUU(4.0f), Height * 0.5f), TEXT("MonsterInspiredWall"), FRotator(0.0f, -42.0f, 0.0f));
    AddCube(MonsterCenter + FVector(-FeetToUU(20.0f), FeetToUU(30.0f), Height + FeetToUU(4.0f)), FVector(FeetToUU(34.0f), FeetToUU(16.0f), FeetToUU(4.0f)), TEXT("MonsterDeck"), FRotator(0.0f, -42.0f, 0.0f));
}

void ATU_StadiumGenerator::GenerateDugouts()
{
    AddCube(FVector(-FeetToUU(55.0f), FeetToUU(85.0f), FeetToUU(4.0f)), FVector(FeetToUU(22.0f), FeetToUU(10.0f), FeetToUU(4.0f)), TEXT("DugoutThirdBase"));
    AddCube(FVector(FeetToUU(55.0f), FeetToUU(85.0f), FeetToUU(4.0f)), FVector(FeetToUU(22.0f), FeetToUU(10.0f), FeetToUU(4.0f)), TEXT("DugoutFirstBase"));
}

void ATU_StadiumGenerator::GenerateLowerBowl()
{
    AddCube(FVector(0.0f, -FeetToUU(110.0f), FeetToUU(10.0f)), FVector(FeetToUU(260.0f), FeetToUU(42.0f), FeetToUU(10.0f)), TEXT("LowerBowlBackstop"));
    AddCube(FVector(-FeetToUU(210.0f), FeetToUU(50.0f), FeetToUU(12.0f)), FVector(FeetToUU(42.0f), FeetToUU(180.0f), FeetToUU(12.0f)), TEXT("LowerBowlLeft"));
    AddCube(FVector(FeetToUU(210.0f), FeetToUU(50.0f), FeetToUU(12.0f)), FVector(FeetToUU(42.0f), FeetToUU(180.0f), FeetToUU(12.0f)), TEXT("LowerBowlRight"));
}

void ATU_StadiumGenerator::GenerateUpperDeck()
{
    AddCube(FVector(0.0f, -FeetToUU(230.0f), FeetToUU(30.0f)), FVector(FeetToUU(210.0f), FeetToUU(24.0f), FeetToUU(8.0f)), TEXT("UpperDeckBackstop"));
    AddCube(FVector(0.0f, -FeetToUU(265.0f), FeetToUU(42.0f)), FVector(FeetToUU(80.0f), FeetToUU(16.0f), FeetToUU(6.0f)), TEXT("PressBox"));
}

void ATU_StadiumGenerator::GenerateExteriorShell()
{
    AddCube(FVector(0.0f, FeetToUU(40.0f), FeetToUU(25.0f)), FVector(FeetToUU(345.0f), FeetToUU(330.0f), FeetToUU(6.0f)), TEXT("ExteriorShellTop"));
    AddCube(FVector(-FeetToUU(350.0f), FeetToUU(80.0f), FeetToUU(15.0f)), FVector(FeetToUU(8.0f), FeetToUU(270.0f), FeetToUU(15.0f)), TEXT("ExteriorShellLeft"));
    AddCube(FVector(FeetToUU(350.0f), FeetToUU(80.0f), FeetToUU(15.0f)), FVector(FeetToUU(8.0f), FeetToUU(270.0f), FeetToUU(15.0f)), TEXT("ExteriorShellRight"));

    AddCube(FVector(0.0f, -FeetToUU(320.0f), FeetToUU(6.0f)), FVector(FeetToUU(80.0f), FeetToUU(12.0f), FeetToUU(6.0f)), TEXT("ConcourseEntryTunnel"));
    AddCube(FVector(FeetToUU(330.0f), FeetToUU(160.0f), FeetToUU(6.0f)), FVector(FeetToUU(16.0f), FeetToUU(20.0f), FeetToUU(6.0f)), TEXT("RightFieldGate"));
}

void ATU_StadiumGenerator::GenerateGameplayMarkers()
{
    AddMarker(FVector(0.0f, -FeetToUU(330.0f), 80.0f), FRotator::ZeroRotator, FLinearColor::Green, TEXT("PlayerStart_ConcourseEntry"));
    AddMarker(FVector(0.0f, -FeetToUU(265.0f), FeetToUU(50.0f)), FRotator::ZeroRotator, FLinearColor::Yellow, TEXT("Objective_PressBox"));
    AddMarker(FVector(FeetToUU(340.0f), FeetToUU(170.0f), 80.0f), FRotator(0.0f, 180.0f, 0.0f), FLinearColor::Blue, TEXT("Extraction_RightFieldGate"));

    AddMarker(FVector(-FeetToUU(120.0f), -FeetToUU(180.0f), 80.0f), FRotator::ZeroRotator, FLinearColor::Red, TEXT("EnemySpawn_Concourse"));
    AddMarker(FVector(FeetToUU(55.0f), FeetToUU(90.0f), 80.0f), FRotator::ZeroRotator, FLinearColor::Red, TEXT("EnemySpawn_Dugout"));
    AddMarker(FVector(-FeetToUU(80.0f), -FeetToUU(230.0f), FeetToUU(32.0f)), FRotator::ZeroRotator, FLinearColor::Red, TEXT("EnemySpawn_UpperDeck"));
    AddMarker(PolarToField(LeftFieldDistanceFt - 20.0f, -40.0f) + FVector(0.0f, 0.0f, FeetToUU(40.0f)), FRotator::ZeroRotator, FLinearColor::Red, TEXT("EnemySpawn_LeftWallDeck"));
    AddMarker(FVector(FeetToUU(20.0f), -FeetToUU(260.0f), FeetToUU(42.0f)), FRotator::ZeroRotator, FLinearColor::Red, TEXT("EnemySpawn_PressBox"));

    AddMarker(FVector(-FeetToUU(180.0f), FeetToUU(40.0f), FeetToUU(14.0f)), FRotator::ZeroRotator, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), TEXT("Patrol_LowerBowl"));
    AddMarker(FVector(0.0f, -FeetToUU(190.0f), FeetToUU(8.0f)), FRotator::ZeroRotator, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), TEXT("Patrol_Concourse"));
    AddMarker(FVector(0.0f, FeetToUU(130.0f), 80.0f), FRotator::ZeroRotator, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), TEXT("Patrol_Field"));
}
