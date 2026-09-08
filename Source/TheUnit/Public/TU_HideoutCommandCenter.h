#pragma once

#include "CoreMinimal.h"
#include "TU_CommandCenterGenerator.h"
#include "TU_HideoutCommandCenter.generated.h"

class UChildActorComponent;
class ATU_HideoutCommandCenterDecorator;
class UTUHideoutProgressionComponent;

/**
 * Preferred pre-mission hub actor: existing functional command center plus the
 * lived-in, progression-aware hideout environment layer.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_HideoutCommandCenter : public ATU_CommandCenterGenerator
{
    GENERATED_BODY()

public:
    ATU_HideoutCommandCenter();

    UFUNCTION(BlueprintPure, Category="Hideout")
    ATU_HideoutCommandCenterDecorator* GetHideoutLayer() const;

    UFUNCTION(BlueprintPure, Category="Hideout")
    UTUHideoutProgressionComponent* GetHideoutProgression() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hideout")
    TObjectPtr<UChildActorComponent> HideoutLayerComponent;
};
