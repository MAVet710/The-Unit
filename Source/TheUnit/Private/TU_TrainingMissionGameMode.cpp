#include "TU_TrainingMissionGameMode.h"

#include "TU_ExtractionZone.h"
#include "TU_KillhouseGenerator.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

ATU_TrainingMissionGameMode::ATU_TrainingMissionGameMode()
{
    MissionStructureClass = ATU_KillhouseGenerator::StaticClass();
    ExtractionZoneClass = ATU_ExtractionZone::StaticClass();
}

void ATU_TrainingMissionGameMode::StartPlay()
{
    if (UWorld* World = GetWorld())
    {
        bool bHasStructure = false;
        for (TActorIterator<ATU_KillhouseGenerator> It(World); It; ++It)
        {
            bHasStructure = true;
            break;
        }

        if (!bHasStructure && MissionStructureClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            World->SpawnActor<ATU_KillhouseGenerator>(MissionStructureClass, MissionStructureTransform, Params);
        }

        bool bHasExtraction = false;
        for (TActorIterator<ATU_ExtractionZone> It(World); It; ++It)
        {
            bHasExtraction = true;
            break;
        }

        if (!bHasExtraction && ExtractionZoneClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            World->SpawnActor<ATU_ExtractionZone>(ExtractionZoneClass, ExtractionTransform, Params);
        }
    }

    Super::StartPlay();
}

AActor* ATU_TrainingMissionGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<APlayerStart> It(World); It; ++It)
        {
            return *It;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (APlayerStart* Spawned = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FallbackPlayerStartTransform, Params))
        {
            return Spawned;
        }
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}
