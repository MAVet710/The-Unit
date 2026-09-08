#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TUMX50TabletComponent.h"
#include "TUBriefingWidget.generated.h"

class ATU_ArmedOperatorCharacter;
class SVerticalBox;

/** Native prototype UI rendered while the operator raises the chest-mounted MX50. */
UCLASS(Blueprintable)
class THEUNIT_API UTUBriefingWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    UFUNCTION(BlueprintCallable, Category="MX50")
    void Configure(ATU_ArmedOperatorCharacter* InOperator, FName InMissionId, const FText& InMissionTitle);

    UFUNCTION(BlueprintCallable, Category="MX50")
    void Refresh();

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetPage(ETUMX50Page Page);

    UFUNCTION(BlueprintPure, Category="MX50")
    ETUMX50Page GetPage() const;

private:
    void RebuildContent();
    void AddNavigation();
    void AddCurrentPageContent();
    FText GetPageLabel(ETUMX50Page Page) const;

    TWeakObjectPtr<ATU_ArmedOperatorCharacter> Operator;
    TSharedPtr<SVerticalBox> RootBox;
};
