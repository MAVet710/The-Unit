#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TUHideoutProgressionComponent.h"
#include "TUHideoutSaveGame.generated.h"

/** Persistent local profile for the pre-mission headquarters and operator selections. */
UCLASS()
class THEUNIT_API UTUHideoutSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Profile")
    int32 SaveVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Hideout")
    TArray<FTUHideoutModuleState> HideoutModules;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName PrimaryId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName SecondaryId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName EquipmentId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Loadout")
    FName MeleeId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    FName ActiveMissionId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    FName HideoutMapName = TEXT("CommandCenter");

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    bool bMissionInProgress = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mission")
    int32 CompletedOperations = 0;
};
