#pragma once

#include "CoreMinimal.h"
#include "TU_GameMode.h"
#include "TU_TrainingMissionGameMode.generated.h"

class ATU_ExtractionZone;
class ATU_KillhouseGenerator;

/** Bootstraps a procedural killhouse training mission into an otherwise blank mission map. */
UCLASS()
class THEUNIT_API ATU_TrainingMissionGameMode : public ATU_GameMode
{
    GENERATED_BODY()

public:
    ATU_TrainingMissionGameMode();
    virtual void StartPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission")
    TSubclassOf<ATU_KillhouseGenerator> MissionStructureClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission")
    TSubclassOf<ATU_ExtractionZone> ExtractionZoneClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Spawn")
    FTransform MissionStructureTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Spawn")
    FTransform ExtractionTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(0.0f, 1950.0f, 100.0f),
        FVector::OneVector);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Spawn")
    FTransform FallbackPlayerStartTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(0.0f, -1950.0f, 100.0f),
        FVector::OneVector);
};
