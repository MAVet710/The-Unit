#include "TU_DonetskEnvironmentDressing.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureLayer(UHierarchicalInstancedStaticMeshComponent* Layer, UStaticMesh* Mesh, bool bCollision, int32 StartCull, int32 EndCull)
{
    if (!Layer) return;
    Layer->SetStaticMesh(Mesh);
    Layer->SetMobility(EComponentMobility::Static);
    Layer->SetCastShadow(true);
    Layer->SetCullDistances(StartCull, EndCull);
    Layer->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    Layer->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Layer->SetCanEverAffectNavigation(bCollision);
}
}

ATU_DonetskEnvironmentDressing::ATU_DonetskEnvironmentDressing()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Static);
    SetRootComponent(Root);

    LampPosts = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LampPosts"));
    RoadDashes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RoadDashes"));
    Barriers = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Barriers"));
    Sedans = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Sedans"));
    Dumpsters = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Dumpsters"));
    BusShelters = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BusShelters"));
    UtilityCabinets = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("UtilityCabinets"));
    Kiosks = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Kiosks"));
    RubblePiles = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RubblePiles"));
    Pallets = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Pallets"));
    TrafficSigns = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TrafficSigns"));
    Benches = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Benches"));
    Planters = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Planters"));
    Crates = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Crates"));
    Manholes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Manholes"));
    Puddles = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Puddles"));

    UHierarchicalInstancedStaticMeshComponent* DressingLayers[] = {
        LampPosts, RoadDashes, Barriers, Sedans, Dumpsters, BusShelters, UtilityCabinets, Kiosks,
        RubblePiles, Pallets, TrafficSigns, Benches, Planters, Crates, Manholes, Puddles
    };
    for (UHierarchicalInstancedStaticMeshComponent* Layer : DressingLayers)
        Layer->SetupAttachment(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Lamp(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_LampPost.SM_Donetsk_Prop_LampPost"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Dash(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_RoadDash.SM_Donetsk_Prop_RoadDash"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Barrier(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_ConcreteBarrier.SM_Donetsk_Prop_ConcreteBarrier"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sedan(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Sedan.SM_Donetsk_Prop_Sedan"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Dumpster(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Dumpster.SM_Donetsk_Prop_Dumpster"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shelter(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_BusShelter.SM_Donetsk_Prop_BusShelter"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cabinet(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_UtilityCabinet.SM_Donetsk_Prop_UtilityCabinet"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Kiosk(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Kiosk.SM_Donetsk_Prop_Kiosk"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Rubble(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_RubblePile.SM_Donetsk_Prop_RubblePile"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Pallet(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Pallet.SM_Donetsk_Prop_Pallet"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sign(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_TrafficSign.SM_Donetsk_Prop_TrafficSign"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Bench(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Bench.SM_Donetsk_Prop_Bench"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Planter(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Planter.SM_Donetsk_Prop_Planter"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Crate(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Crate.SM_Donetsk_Prop_Crate"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Manhole(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Manhole.SM_Donetsk_Prop_Manhole"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Puddle(TEXT("/Game/TheUnit/Donetsk/Environment/SM_Donetsk_Prop_Puddle.SM_Donetsk_Prop_Puddle"));

    ConfigureLayer(LampPosts, Lamp.Object, true, 3500, 70000);
    ConfigureLayer(RoadDashes, Dash.Object, false, 1500, 50000);
    ConfigureLayer(Barriers, Barrier.Object, true, 2500, 65000);
    ConfigureLayer(Sedans, Sedan.Object, true, 3500, 70000);
    ConfigureLayer(Dumpsters, Dumpster.Object, true, 2500, 55000);
    ConfigureLayer(BusShelters, Shelter.Object, true, 3500, 65000);
    ConfigureLayer(UtilityCabinets, Cabinet.Object, true, 2000, 50000);
    ConfigureLayer(Kiosks, Kiosk.Object, true, 3000, 60000);
    ConfigureLayer(RubblePiles, Rubble.Object, true, 2000, 50000);
    ConfigureLayer(Pallets, Pallet.Object, true, 1500, 40000);
    ConfigureLayer(TrafficSigns, Sign.Object, true, 2500, 55000);
    ConfigureLayer(Benches, Bench.Object, true, 2000, 45000);
    ConfigureLayer(Planters, Planter.Object, true, 2000, 45000);
    ConfigureLayer(Crates, Crate.Object, true, 1500, 40000);
    ConfigureLayer(Manholes, Manhole.Object, false, 1000, 35000);
    ConfigureLayer(Puddles, Puddle.Object, false, 1000, 28000);
}

void ATU_DonetskEnvironmentDressing::Add(
    UHierarchicalInstancedStaticMeshComponent* Layer,
    const FVector& Location,
    float YawDegrees,
    const FVector& Scale)
{
    if (!Layer || !Layer->GetStaticMesh()) return;
    Layer->AddInstance(FTransform(FRotator(0.0f, YawDegrees, 0.0f), Location, Scale), false);
}

void ATU_DonetskEnvironmentDressing::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildDressing();
}

void ATU_DonetskEnvironmentDressing::RebuildDressing()
{
    UHierarchicalInstancedStaticMeshComponent* DressingLayers[] = {
        LampPosts, RoadDashes, Barriers, Sedans, Dumpsters, BusShelters, UtilityCabinets, Kiosks,
        RubblePiles, Pallets, TrafficSigns, Benches, Planters, Crates, Manholes, Puddles
    };
    for (UHierarchicalInstancedStaticMeshComponent* Layer : DressingLayers)
        if (Layer) Layer->ClearInstances();

    FRandomStream Random(0xD07E57);

    // Long boulevard rhythm. Keep the center open for vehicles, AI and extraction traversal.
    for (int32 Y = -25200; Y <= 25200; Y += 2800)
    {
        Add(LampPosts, FVector(-3020.0f, static_cast<float>(Y), 0.0f), 90.0f);
        Add(LampPosts, FVector(3020.0f, static_cast<float>(Y + 1400), 0.0f), -90.0f);

        if ((Y / 2800) % 2 == 0)
        {
            Add(TrafficSigns, FVector(-2440.0f, static_cast<float>(Y + 720), 0.0f), 90.0f);
            Add(TrafficSigns, FVector(2440.0f, static_cast<float>(Y - 720), 0.0f), -90.0f);
        }
    }

    // Broken white lane dashes on the main boulevard and both cross streets.
    for (int32 Y = -28600; Y <= 28600; Y += 900)
    {
        Add(RoadDashes, FVector(-760.0f, static_cast<float>(Y), 12.0f), 0.0f);
        Add(RoadDashes, FVector(760.0f, static_cast<float>(Y + 450), 12.0f), 0.0f);
    }
    for (int32 X = -23500; X <= 23500; X += 1000)
    {
        Add(RoadDashes, FVector(static_cast<float>(X), -9500.0f, 12.0f), 90.0f);
        Add(RoadDashes, FVector(static_cast<float>(X + 500), 10200.0f, 12.0f), 90.0f);
    }

    const FVector2D ParkedCars[] = {
        {-2380,-16100}, {2420,-13700}, {-2450,-8200}, {2450,-5400},
        {-2390,1800}, {2420,5200}, {-2460,13600}, {2440,17600},
        {-9800,7900}, {-12700,11200}, {-15100,8500},
        {6800,24700}, {11800,24700}, {-14100,-13200}, {-10300,-15400}
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ParkedCars); ++Index)
    {
        const FVector2D P = ParkedCars[Index];
        const float Yaw = (Index >= 8 && Index <= 10) ? Random.FRandRange(-8.0f, 8.0f)
            : ((Index >= 11 && Index <= 12) ? 90.0f : Random.FRandRange(-3.5f, 3.5f));
        const float Uniform = Random.FRandRange(0.94f, 1.04f);
        Add(Sedans, FVector(P.X, P.Y, 0.0f), Yaw, FVector(Uniform));
    }

    // Public transport stops and ordinary service clutter.
    Add(BusShelters, FVector(2680,-7200,0), 180.0f);
    Add(BusShelters, FVector(-2680,7100,0), 0.0f);
    Add(Kiosks, FVector(-3150,3400,0), 90.0f);
    Add(Kiosks, FVector(-3150,3950,0), 90.0f);
    Add(Kiosks, FVector(3600,-11800,0), -90.0f);
    Add(Kiosks, FVector(16400,18500,0), 180.0f);

    const FVector Cabinets[] = {
        FVector(2950,11800,0), FVector(-3100,-4300,0), FVector(-10200,5900,0),
        FVector(16400,8800,0), FVector(-18400,21900,0), FVector(-11300,-11800,0)
    };
    for (int32 Index=0; Index<UE_ARRAY_COUNT(Cabinets); ++Index)
        Add(UtilityCabinets, Cabinets[Index], (Index & 1) ? 90.0f : 0.0f);

    const FVector DumpstersPos[] = {
        FVector(-11900,12600,0), FVector(-15100,10600,0), FVector(18400,10900,0),
        FVector(-13700,-12400,0), FVector(-17800,-16800,0), FVector(8900,27600,0)
    };
    for (int32 Index=0; Index<UE_ARRAY_COUNT(DumpstersPos); ++Index)
        Add(Dumpsters, DumpstersPos[Index], Random.FRandRange(-25.0f,25.0f));

    // Civic square human scale.
    for (int32 Row=-3; Row<=3; ++Row)
    {
        const float Y = Row * 5200.0f;
        Add(Benches, FVector(4300,Y,0), 90.0f);
        Add(Benches, FVector(15500,Y+1800.0f,0), -90.0f);
    }
    for (int32 Row=-4; Row<=4; ++Row)
    {
        const float Y = Row * 4300.0f;
        Add(Planters, FVector(5200,Y,0), 0.0f, FVector(0.9f));
        Add(Planters, FVector(14800,Y+1100.0f,0), 0.0f, FVector(0.9f));
    }

    // Industrial yard loading clutter.
    for (int32 I=0; I<9; ++I)
    {
        Add(Pallets, FVector(-19100.0f + I*760.0f, -17100.0f + (I%3)*310.0f, 0.0f), Random.FRandRange(-12.0f,12.0f));
        if ((I % 2)==0)
            Add(Crates, FVector(-18750.0f + I*720.0f, -16200.0f + (I%2)*420.0f, 0.0f), Random.FRandRange(-20.0f,20.0f));
    }

    // Original raid-state damage. These positions deliberately avoid both extraction corridors.
    const FVector Rubble[] = {
        FVector(-5150,-17600,0), FVector(15500,-14100,0), FVector(5350,16700,0),
        FVector(-3300,-23100,0), FVector(3380,-23800,0), FVector(-17600,-13200,0)
    };
    for (int32 Index=0; Index<UE_ARRAY_COUNT(Rubble); ++Index)
        Add(RubblePiles, Rubble[Index], Random.FRandRange(-40.0f,40.0f), FVector(Random.FRandRange(0.82f,1.18f)));

    for (int32 I=0; I<5; ++I)
        Add(Barriers, FVector(-1100.0f + I*560.0f, -20550.0f + (I%2)*190.0f, 0.0f), (I%2) ? 8.0f : -6.0f);

    // Road-surface detail is non-colliding and deliberately sparse.
    for (int32 Y=-23800; Y<=23800; Y+=4800)
    {
        Add(Manholes, FVector(-420.0f,static_cast<float>(Y),13.0f), Random.FRandRange(0.0f,360.0f));
        if ((Y/4800)&1)
            Add(Puddles, FVector(1160.0f,static_cast<float>(Y+900),14.0f), Random.FRandRange(-12.0f,12.0f),
                FVector(Random.FRandRange(0.75f,1.35f), Random.FRandRange(0.7f,1.2f), 1.0f));
    }
}
