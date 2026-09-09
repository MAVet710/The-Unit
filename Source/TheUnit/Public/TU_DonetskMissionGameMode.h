#pragma once

#include "CoreMinimal.h"
#include "TU_GameMode.h"
#include "TU_DonetskMissionGameMode.generated.h"

class ATU_DonetskDistrictGenerator;
class ATU_ExtractionZone;

/** Bootstraps the Donetsk reference district into an otherwise blank mission map. */
UCLASS()
class THEUNIT_API ATU_DonetskMissionGameMode : public ATU_GameMode
{
    GENERATED_BODY()

public:
    ATU_DonetskMissionGameMode();
    virtual void StartPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Donetsk")
    TSubclassOf<ATU_DonetskDistrictGenerator> DistrictClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Donetsk")
    TSubclassOf<ATU_ExtractionZone> ExtractionZoneClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Donetsk")
    FTransform DistrictTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Donetsk")
    FTransform ExtractionTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(0.0f, 27200.0f, 100.0f),
        FVector::OneVector);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Donetsk")
    FTransform FallbackPlayerStartTransform = FTransform(
        FRotator::ZeroRotator,
        FVector(0.0f, -27500.0f, 110.0f),
        FVector::OneVector);
};