#pragma once

#include "CoreMinimal.h"
#include "TU_GameMode.h"
#include "TU_HideoutGameMode.generated.h"

class ATU_HideoutCommandCenter;

/** Bootstraps the complete pre-mission hideout into an otherwise blank CommandCenter map. */
UCLASS()
class THEUNIT_API ATU_HideoutGameMode : public ATU_GameMode
{
    GENERATED_BODY()

public:
    ATU_HideoutGameMode();
    virtual void StartPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hideout")
    TSubclassOf<ATU_HideoutCommandCenter> HideoutClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hideout|Spawn")
    FTransform FallbackPlayerStartTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(0.0f, -3300.0f, 100.0f),
        FVector::OneVector);
};
