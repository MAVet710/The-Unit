#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TUMX50TabletComponent.h"
#include "TUBriefingWidget.generated.h"

class ATU_ArmedOperatorCharacter;
class SVerticalBox;
class UTUMX50TabletComponent;

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
    void SetMissionSnapshot(const FTMX50MissionSnapshot& InSnapshot);

    UFUNCTION(BlueprintCallable, Category="MX50")
    void Refresh();

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetPage(ETUMX50Page Page);

    UFUNCTION(BlueprintPure, Category="MX50")
    ETUMX50Page GetPage() const { return ActivePage; }

    UFUNCTION(BlueprintPure, Category="MX50")
    FTMX50MissionSnapshot GetMissionSnapshot() const { return MissionSnapshot; }

private:
    void RebuildContent();
    void AddNavigation();
    void AddCurrentPageContent();
    void AddMapPage();
    void AddVideoPage();
    void SelectMapMarker(FName MarkerId);
    void SelectVideoFeed(FName FeedId);
    UTUMX50TabletComponent* GetTabletState() const;
    FText GetPageLabel(ETUMX50Page Page) const;
    FText GetMarkerTypeLabel(ETUMX50MapMarkerType Type) const;
    FText GetVideoFeedTypeLabel(ETUMX50VideoFeedType Type) const;

    TWeakObjectPtr<ATU_ArmedOperatorCharacter> Operator;
    TSharedPtr<SVerticalBox> RootBox;
    FTMX50MissionSnapshot MissionSnapshot;
    ETUMX50Page ActivePage = ETUMX50Page::Mission;
};
