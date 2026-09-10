#include "TU_HideoutGameMode.h"

#include "TU_HideoutCommandCenter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

ATU_HideoutGameMode::ATU_HideoutGameMode()
{
    HideoutClass = ATU_HideoutCommandCenter::StaticClass();
}

void ATU_HideoutGameMode::StartPlay()
{
    if (UWorld* World = GetWorld())
    {
        bool bHasHideout = false;
        for (TActorIterator<ATU_HideoutCommandCenter> It(World); It; ++It)
        {
            bHasHideout = true;
            break;
        }

        if (!bHasHideout && HideoutClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            World->SpawnActor<ATU_HideoutCommandCenter>(HideoutClass, FTransform::Identity, Params);
        }
    }

    Super::StartPlay();
}

AActor* ATU_HideoutGameMode::ChoosePlayerStart_Implementation(AController* Player)
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
