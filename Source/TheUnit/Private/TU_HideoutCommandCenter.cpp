#include "TU_HideoutCommandCenter.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutProgressionComponent.h"
#include "TU_CommandCenterStation.h"
#include "TU_HideoutCommandCenterDecorator.h"
#include "TU_HideoutUpgradeStation.h"
#include "Components/ChildActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ATU_HideoutCommandCenter::ATU_HideoutCommandCenter()
{
    HideoutLayerComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("HideoutLayer"));
    HideoutLayerComponent->SetupAttachment(GetRootComponent());
    HideoutLayerComponent->SetChildActorClass(ATU_HideoutCommandCenterDecorator::StaticClass());
}

void ATU_HideoutCommandCenter::BeginPlay()
{
    Super::BeginPlay();
    RestorePersistentState();
    WireMissionStations();
    if (bSpawnUpgradeStations)
    {
        SpawnUpgradeStations();
    }
}

void ATU_HideoutCommandCenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CapturePersistentState();
    ClearUpgradeStations();
    Super::EndPlay(EndPlayReason);
}

ATU_HideoutCommandCenterDecorator* ATU_HideoutCommandCenter::GetHideoutLayer() const
{
    return HideoutLayerComponent
        ? Cast<ATU_HideoutCommandCenterDecorator>(HideoutLayerComponent->GetChildActor())
        : nullptr;
}

UTUHideoutProgressionComponent* ATU_HideoutCommandCenter::GetHideoutProgression() const
{
    if (ATU_HideoutCommandCenterDecorator* Layer = GetHideoutLayer())
    {
        return Layer->GetProgression();
    }
    return nullptr;
}

void ATU_HideoutCommandCenter::RestorePersistentState()
{
    UGameInstance* GameInstance = GetGameInstance();
    UTUHideoutProgressionComponent* Progression = GetHideoutProgression();
    if (!GameInstance || !Progression)
    {
        return;
    }

    if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
    {
        Lifecycle->SetHideoutMapName(HideoutMapName);
        Lifecycle->ApplyHideoutState(Progression);
        if (ATU_HideoutCommandCenterDecorator* Layer = GetHideoutLayer())
        {
            Layer->RefreshFromProgression();
        }
    }
}

void ATU_HideoutCommandCenter::CapturePersistentState()
{
    UGameInstance* GameInstance = GetGameInstance();
    UTUHideoutProgressionComponent* Progression = GetHideoutProgression();
    if (!GameInstance || !Progression)
    {
        return;
    }

    if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
    {
        Lifecycle->CaptureHideoutState(Progression);
        Lifecycle->SaveProfile();
    }
}

void ATU_HideoutCommandCenter::WireMissionStations()
{
    if (!DefaultMissionPackage || !GetWorld())
    {
        return;
    }

    for (TActorIterator<ATU_CommandCenterStation> It(GetWorld()); It; ++It)
    {
        ATU_CommandCenterStation* Station = *It;
        if (!Station || Station->GetOwner() != this)
        {
            continue;
        }

        const ETUCommandCenterStationType Type = Station->GetStationType();
        if (Type == ETUCommandCenterStationType::Briefing || Type == ETUCommandCenterStationType::MissionLaunch)
        {
            Station->SetMissionPackage(DefaultMissionPackage);
        }
    }
}

void ATU_HideoutCommandCenter::ClearUpgradeStations()
{
    for (ATU_HideoutUpgradeStation* Station : UpgradeStations)
    {
        if (IsValid(Station))
        {
            Station->Destroy();
        }
    }
    UpgradeStations.Reset();
}

void ATU_HideoutCommandCenter::SpawnUpgradeStations()
{
    if (!GetWorld())
    {
        return;
    }
    ClearUpgradeStations();

    struct FUpgradeSpawn
    {
        ETUHideoutModuleType Type;
        FVector LocalLocation;
        const TCHAR* Label;
    };

    const FUpgradeSpawn Spawns[] = {
        { ETUHideoutModuleType::Power, FVector(1750.0f, 2550.0f, 65.0f), TEXT("POWER // UPGRADE") },
        { ETUHideoutModuleType::Communications, FVector(1950.0f, 150.0f, 65.0f), TEXT("COMMS // UPGRADE") },
        { ETUHideoutModuleType::Medical, FVector(1900.0f, 1550.0f, 65.0f), TEXT("MEDICAL // UPGRADE") },
        { ETUHideoutModuleType::Storage, FVector(-1950.0f, 1750.0f, 65.0f), TEXT("STORAGE // UPGRADE") },
        { ETUHideoutModuleType::ArmorySupport, FVector(-1700.0f, -2450.0f, 65.0f), TEXT("ARMORY SUPPORT // UPGRADE") },
        { ETUHideoutModuleType::GearMaintenance, FVector(-1100.0f, -2850.0f, 65.0f), TEXT("GEAR MAINTENANCE // UPGRADE") },
        { ETUHideoutModuleType::Planning, FVector(1650.0f, 700.0f, 65.0f), TEXT("PLANNING // UPGRADE") },
        { ETUHideoutModuleType::RangeSupport, FVector(2350.0f, -2750.0f, 65.0f), TEXT("RANGE SUPPORT // UPGRADE") },
    };

    for (const FUpgradeSpawn& Spawn : Spawns)
    {
        const FVector WorldLocation = GetActorTransform().TransformPosition(Spawn.LocalLocation);
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ATU_HideoutUpgradeStation* Station = GetWorld()->SpawnActor<ATU_HideoutUpgradeStation>(
            ATU_HideoutUpgradeStation::StaticClass(), WorldLocation, GetActorRotation(), Params);
        if (Station)
        {
            Station->ConfigureUpgradeStation(Spawn.Type, FText::FromString(Spawn.Label));
            UpgradeStations.Add(Station);
        }
    }
}
