#include "TU_DonetskMissionGameMode.h"

#include "TU_DonetskDistrictGenerator.h"
#include "TU_DonetskEnvironmentDressing.h"
#include "TU_ExtractionZone.h"
#include "TU_ObjectiveBase.h"
#include "TU_RaidCombatant.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/Engine.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

ATU_DonetskMissionGameMode::ATU_DonetskMissionGameMode()
{
    DistrictClass = ATU_DonetskDistrictGenerator::StaticClass();
    ExtractionZoneClass = ATU_ExtractionZone::StaticClass();
    bTrainingRaid=false;
    FTUTaskDefinition Relay; Relay.TaskId=TEXT("RelaySurvey"); Relay.Policy=ETUTaskPolicy::SameRaid;
    FTUTaskStep Visit; Visit.Condition=ETUTaskCondition::Visit; Visit.TargetId=TEXT("RelayApproach");
    FTUTaskStep Interact; Interact.Condition=ETUTaskCondition::Interact; Interact.TargetId=TEXT("RelayConsole");
    Relay.Steps={Visit,Interact}; DefaultTasks.Add(Relay);
    FTUTaskDefinition Recover; Recover.TaskId=TEXT("RecoverRelay"); Recover.Policy=ETUTaskPolicy::ExtractRequired;
    Recover.Condition=ETUTaskCondition::Recover; Recover.TargetId=TEXT("RelayData"); DefaultTasks.Add(Recover);
    FTUTaskDefinition TurnIn; TurnIn.TaskId=TEXT("DeliverRelay"); TurnIn.Policy=ETUTaskPolicy::PhysicalHandover;
    TurnIn.Condition=ETUTaskCondition::Handover; TurnIn.TargetId=TEXT("RelayData"); DefaultTasks.Add(TurnIn);
    FTUTaskDefinition Optional; Optional.TaskId=TEXT("OptionalPatrol"); Optional.Policy=ETUTaskPolicy::Cumulative;
    Optional.Condition=ETUTaskCondition::Eliminate; Optional.TargetId=TEXT("RaidGuard"); Optional.RequiredCount=3; DefaultTasks.Add(Optional);
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

        bool bHasDressing = false;
        for (TActorIterator<ATU_DonetskEnvironmentDressing> It(World); It; ++It)
        {
            bHasDressing = true;
            break;
        }

        if (!bHasDressing)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            World->SpawnActor<ATU_DonetskEnvironmentDressing>(
                ATU_DonetskEnvironmentDressing::StaticClass(), FTransform::Identity, Params);
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
            if(ATU_ExtractionZone* North=World->SpawnActor<ATU_ExtractionZone>(ExtractionZoneClass, ExtractionTransform, Params)) North->ExtractId=TEXT("NorthRoad");
            if(ATU_ExtractionZone* South=World->SpawnActor<ATU_ExtractionZone>(ExtractionZoneClass,FVector(600.f,-26900.f,120.f),FRotator::ZeroRotator,Params)) South->ExtractId=TEXT("SouthRoad");
        }

        // A small accessible software benchmark within the generated district. These are visible prototype props.
        FActorSpawnParameters ObjectiveParams; ObjectiveParams.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto SpawnObjective=[&](FVector Position,FName Id,ETUTaskCondition Condition)
        {
            ATU_ObjectiveBase* Objective=World->SpawnActor<ATU_ObjectiveBase>(Position,FRotator::ZeroRotator,ObjectiveParams);
            if(!Objective) return;
            Objective->ConfigureObjective(Id,Condition);
        };
        SpawnObjective(FVector(0,-27200,100),TEXT("RelayApproach"),ETUTaskCondition::Visit);
        SpawnObjective(FVector(160,-27000,100),TEXT("RelayConsole"),ETUTaskCondition::Interact);
        SpawnObjective(FVector(300,-26800,100),TEXT("RelayData"),ETUTaskCondition::Recover);
        const FVector AIPositions[6] = {
            FVector(900,-25400,120), FVector(-900,-24900,120), FVector(1500,-24400,120),
            FVector(-1500,-23900,120), FVector(400,-23200,120), FVector(-400,-22600,120)
        };
        for(int32 Index=0;Index<6;++Index)
        {
            if(ATU_RaidCombatant* Guard=World->SpawnActor<ATU_RaidCombatant>(AIPositions[Index],FRotator(0,180,0),ObjectiveParams))
                Guard->Archetype=static_cast<ETUAIArchetype>(Index);
        }
    }

    Super::StartPlay();

    // G7 profiling must begin after the renderer/RHI is initialized. UE 5.7's
    // startup -csvprofile path asserts before RHI init in this packaged build, so
    // the benchmark is deliberately armed from the live Donetsk world instead.
    if (FParse::Param(FCommandLine::Get(), TEXT("TUG7PerfCapture")))
    {
        if (UWorld* PerfWorld = GetWorld())
        {
            const TWeakObjectPtr<UWorld> WeakWorld(PerfWorld);
            FTimerHandle SettingsTimer;
            PerfWorld->GetTimerManager().SetTimer(SettingsTimer, FTimerDelegate::CreateLambda([WeakWorld]()
            {
                if (UWorld* World = WeakWorld.Get(); World && GEngine)
                {
                    GEngine->Exec(World, TEXT("r.DynamicRes.OperationMode 0"));
                    GEngine->Exec(World, TEXT("r.SetRes 1920x1080w"));
                    GEngine->Exec(World, TEXT("r.ScreenPercentage 100"));
                    GEngine->Exec(World, TEXT("r.SecondaryScreenPercentage.GameViewport 100"));
                    GEngine->Exec(World, TEXT("sg.ResolutionQuality 100"));
                    GEngine->Exec(World, TEXT("sg.ViewDistanceQuality 3"));
                    GEngine->Exec(World, TEXT("sg.AntiAliasingQuality 3"));
                    GEngine->Exec(World, TEXT("sg.ShadowQuality 3"));
                    GEngine->Exec(World, TEXT("sg.GlobalIlluminationQuality 3"));
                    GEngine->Exec(World, TEXT("sg.ReflectionQuality 3"));
                    GEngine->Exec(World, TEXT("sg.PostProcessQuality 3"));
                    GEngine->Exec(World, TEXT("sg.TextureQuality 3"));
                    GEngine->Exec(World, TEXT("sg.EffectsQuality 3"));
                    GEngine->Exec(World, TEXT("sg.FoliageQuality 3"));
                    UE_LOG(LogTemp, Display, TEXT("TUG7Perf SettingsApplied 1920x1080 SP100 Quality3"));
                }
            }), 4.0f, false);

            FTimerHandle StartTimer;
            PerfWorld->GetTimerManager().SetTimer(StartTimer, FTimerDelegate::CreateLambda([WeakWorld]()
            {
                if (UWorld* World = WeakWorld.Get(); World && GEngine)
                {
                    GEngine->Exec(World, TEXT("csvprofile start"));
                    UE_LOG(LogTemp, Display, TEXT("TUG7Perf CaptureStarted"));
                }
            }), 7.0f, false);

            FTimerHandle StopTimer;
            PerfWorld->GetTimerManager().SetTimer(StopTimer, FTimerDelegate::CreateLambda([WeakWorld]()
            {
                if (UWorld* World = WeakWorld.Get(); World && GEngine)
                {
                    GEngine->Exec(World, TEXT("csvprofile stop"));
                    UE_LOG(LogTemp, Display, TEXT("TUG7Perf CaptureStopped"));
                }
            }), 37.0f, false);

            FTimerHandle ExitTimer;
            PerfWorld->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateLambda([]()
            {
                UE_LOG(LogTemp, Display, TEXT("TUG7Perf Exit"));
                FGenericPlatformMisc::RequestExit(false);
            }), 40.0f, false);
        }
    }
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
