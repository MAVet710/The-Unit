#pragma once

#include "CoreMinimal.h"
#include "TU_CommandCenterGenerator.h"
#include "TU_HideoutCommandCenter.generated.h"

class UChildActorComponent;
class ATU_HideoutCommandCenterDecorator;
class ATU_HideoutUpgradeStation;
class UTUHideoutProgressionComponent;

/**
 * Preferred pre-mission hub actor: existing functional command center plus the
 * lived-in, progression-aware hideout environment layer and persistent lifecycle.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_HideoutCommandCenter : public ATU_CommandCenterGenerator
{
    GENERATED_BODY()

public:
    ATU_HideoutCommandCenter();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintPure, Category="Hideout")
    ATU_HideoutCommandCenterDecorator* GetHideoutLayer() const;

    UFUNCTION(BlueprintPure, Category="Hideout")
    UTUHideoutProgressionComponent* GetHideoutProgression() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hideout")
    TObjectPtr<UChildActorComponent> HideoutLayerComponent;

    /** Stable travel name used by mission maps to return to this headquarters. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hideout|Travel")
    FName HideoutMapName = TEXT("CommandCenter");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hideout|Upgrades")
    bool bSpawnUpgradeStations = true;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ATU_HideoutUpgradeStation>> UpgradeStations;

    void RestorePersistentState();
    void CapturePersistentState();
    void SpawnUpgradeStations();
    void ClearUpgradeStations();
};
