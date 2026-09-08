#include "TU_HideoutCommandCenter.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutProgressionComponent.h"
#include "TUMissionPackageData.h"
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
    EnsureDefaultMissionPackage();
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

void ATU_HideoutCommandCenter::EnsureDefaultMissionPackage()
{
    if (DefaultMissionPackage || !bCreateFallbackTrainingMission || FallbackTrainingMapName.IsNone())
    {
        return;
    }

    RuntimeFallbackMissionPackage = NewObject<UTUMissionPackageData>(this, TEXT("RuntimeKillhouseMission"));
    if (!RuntimeFallbackMissionPackage)
    {
        return;
    }

    RuntimeFallbackMissionPackage->Mission.MissionId = TEXT("OP_KILLHOUSE");
    RuntimeFallbackMissionPackage->Mission.MissionTitle = FText::FromString(TEXT("Kill House Evaluation"));
    RuntimeFallbackMissionPackage->Mission.Objective = FText::FromString(TEXT("Clear the training structure and extract after completing the evaluation."));
    RuntimeFallbackMissionPackage->Mission.Area = FText::FromString(TEXT("Command Training Facility"));
    RuntimeFallbackMissionPackage->Mission.ThreatSummary = FText::FromString(TEXT("Controlled training threat package. Live intelligence replaces this fallback package for authored operations."));
    RuntimeFallbackMissionPackage->Mission.TeamSummary = FText::FromString(TEXT("Local operator training package. Co-op roster data replaces this fallback when available."));
    RuntimeFallbackMissionPackage->Mission.bDeploymentAuthorized = true;
    RuntimeFallbackMissionPackage->DestinationMap = FallbackTrainingMapName;

    FTMX50MapMarker Entry;
    Entry.MarkerId = TEXT("ENTRY_START");
    Entry.Label = FText::FromString(TEXT("Start / Entry"));
    Entry.Type = ETUMX50MapMarkerType::Entry;
    Entry.NormalizedPosition = FVector2D(0.50f, 0.92f);
    Entry.Details = FText::FromString(TEXT("Training start position."));
    RuntimeFallbackMissionPackage->MapMarkers.Add(Entry);

    FTMX50MapMarker Objective;
    Objective.MarkerId = TEXT("OBJ_CLEAR");
    Objective.Label = FText::FromString(TEXT("Clear Structure"));
    Objective.Type = ETUMX50MapMarkerType::Objective;
    Objective.NormalizedPosition = FVector2D(0.50f, 0.48f);
    Objective.Details = FText::FromString(TEXT("Complete the assigned kill-house evaluation."));
    RuntimeFallbackMissionPackage->MapMarkers.Add(Objective);

    FTMX50MapMarker Extraction;
    Extraction.MarkerId = TEXT("EXTRACT_END");
    Extraction.Label = FText::FromString(TEXT("Extraction"));
    Extraction.Type = ETUMX50MapMarkerType::Extraction;
    Extraction.NormalizedPosition = FVector2D(0.50f, 0.08f);
    Extraction.Details = FText::FromString(TEXT("Return to headquarters after the evaluation."));
    RuntimeFallbackMissionPackage->MapMarkers.Add(Extraction);

    RuntimeFallbackMissionPackage->PlannedRoute = {
        Entry.NormalizedPosition,
        Objective.NormalizedPosition,
        Extraction.NormalizedPosition
    };

    DefaultMissionPackage = RuntimeFallbackMissionPackage;
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
