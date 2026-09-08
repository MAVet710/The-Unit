#pragma once

#include "CoreMinimal.h"
#include "TU_CommandCenterStation.h"
#include "TUHideoutProgressionComponent.h"
#include "TU_HideoutUpgradeStation.generated.h"

/** Physical interaction point for one upgradeable hideout module. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_HideoutUpgradeStation : public ATU_CommandCenterStation
{
    GENERATED_BODY()

public:
    ATU_HideoutUpgradeStation();

    UFUNCTION(BlueprintCallable, Category="Hideout|Upgrade")
    void ConfigureUpgradeStation(ETUHideoutModuleType InModuleType, const FText& InLabel);

    UFUNCTION(BlueprintPure, Category="Hideout|Upgrade")
    ETUHideoutModuleType GetModuleType() const { return ModuleType; }

    virtual bool UseStation(ATU_ArmedOperatorCharacter* Operator) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hideout|Upgrade")
    ETUHideoutModuleType ModuleType = ETUHideoutModuleType::Power;
};
