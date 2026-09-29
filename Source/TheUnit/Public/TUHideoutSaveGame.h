#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TUEquipmentTypes.h"
#include "TUExecutionTypes.h"
#include "TUHideoutProgressionComponent.h"
#include "TUHideoutSaveGame.generated.h"

/** Persistent local profile for the pre-mission headquarters and operator selections. */
UCLASS()
class THEUNIT_API UTUHideoutSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Profile")
    int32 SaveVersion = 3;

    UPROPERTY(SaveGame) FGuid LocalPlayerId;
    UPROPERTY(SaveGame) int32 CommitSequence = 0;
    UPROPERTY(SaveGame) bool bInitialKitCaptured = false;
    UPROPERTY(SaveGame) TArray<FGuid> InitializedPlayers;
    UPROPERTY(SaveGame) TMap<FString, FGuid> KnownPlayers;
    UPROPERTY(SaveGame) FTUItemLedger Stash;
    // Only LooseCartridges is used; keyed because fungible cartridges have no instance OwnerId.
    UPROPERTY(SaveGame) TMap<FGuid, FTUItemLedger> LooseByPlayer;
    UPROPERTY(SaveGame) TArray<FTUDeploymentRecord> Deployments;
    UPROPERTY(SaveGame) TArray<FTURaidOutcome> Outcomes;
    UPROPERTY(SaveGame) TArray<FTUTaskProgress> Tasks;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Hideout")
    TArray<FTUHideoutModuleState> HideoutModules;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName PrimaryId = NAME_None;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName SecondaryId = NAME_None;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName EquipmentId = NAME_None;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName MeleeId = NAME_None;

    /** Stable modular equipment item id saved independently for each tactical slot. */
    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Loadout|Gear")
    TMap<ETUEquipmentSlot, FName> GearBySlot;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    FName ActiveMissionId = NAME_None;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    FName HideoutMapName = TEXT("CommandCenter");

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    bool bMissionInProgress = false;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    int32 CompletedOperations = 0;
};
