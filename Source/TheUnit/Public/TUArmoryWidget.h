#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TUArmoryWidget.generated.h"

class ATU_ArmedOperatorCharacter;
class SVerticalBox;

/** Native prototype armory UI for Primary, Secondary, Melee and Equipment slots. */
UCLASS(Blueprintable)
class THEUNIT_API UTUArmoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    UFUNCTION(BlueprintCallable, Category="Armory")
    void SetOperator(ATU_ArmedOperatorCharacter* InOperator);

    UFUNCTION(BlueprintCallable, Category="Armory")
    void Refresh();

private:
    void RebuildContent();
    void AddSectionHeader(const FString& Label);
    void AddPrimaryChoices();
    void AddSecondaryChoices();
    void AddMeleeChoices();
    void AddEquipmentChoices();
    void AddSummary();

    TWeakObjectPtr<ATU_ArmedOperatorCharacter> Operator;
    TSharedPtr<SVerticalBox> RootBox;
};
