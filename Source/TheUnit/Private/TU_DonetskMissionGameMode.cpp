#include "TU_DonetskMissionGameMode.h"

#include "TU_DonetskDistrictGenerator.h"
#include "TU_ExtractionZone.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

ATU_DonetskMissionGameMode::ATU_DonetskMissionGameMode()
{
    DistrictClass = ATU_DonetskDistrictGenerator::StaticClass();
    ExtractionZoneClass = ATU_ExtractionZone::StaticClass();
}

void ATU_DonetskMissionGameMode::StartPlay()
{
    if (UWorld* World = GetWorld())
    {
        bool bHasDistrict = false;
        for (TActorIterator<ATU_DonetskDistrictGenerator> It(World); It; ++It)
        {
            bHasDistrict = true;
            break;
        }

        if (!bHasDistrict && DistrictClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            World->SpawnActor<ATU_DonetskDistrictGenerator>(DistrictClass, DistrictTransform, Params);
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

AActor* ATU_DonetskMissionGameMode::ChoosePlayerStart_Implementation(AController* Player)
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