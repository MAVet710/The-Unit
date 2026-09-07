#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TUBriefingWidget.generated.h"

class ATU_ArmedOperatorCharacter;
class SVerticalBox;

/** Native prototype mission board used by the physical Briefing Room. */
UCLASS(Blueprintable)
class THEUNIT_API UTUBriefingWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    UFUNCTION(BlueprintCallable, Category="Briefing")
    void Configure(ATU_ArmedOperatorCharacter* InOperator, FName InMissionId, const FText& InMissionTitle);

    UFUNCTION(BlueprintCallable, Category="Briefing")
    void Refresh();

private:
    void RebuildContent();

    TWeakObjectPtr<ATU_ArmedOperatorCharacter> Operator;
    TSharedPtr<SVerticalBox> RootBox;
    FName MissionId = TEXT("OP_KILLHOUSE");
    FText MissionTitle = FText::FromString(TEXT("Kill House Evaluation"));
};
