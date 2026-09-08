#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUMX50TabletComponent.generated.h"

class UTUMissionPackageData;

UENUM(BlueprintType)
enum class ETUMX50Page : uint8
{
    Mission UMETA(DisplayName="Mission"),
    Map UMETA(DisplayName="Map"),
    Intel UMETA(DisplayName="Intel"),
    Team UMETA(DisplayName="Team"),
    Drone UMETA(DisplayName="Drone / Video"),
    Loadout UMETA(DisplayName="Loadout / Status")
};

UENUM(BlueprintType)
enum class ETUMX50MapMarkerType : uint8
{
    Entry UMETA(DisplayName="Entry"),
    Objective UMETA(DisplayName="Objective"),
    Threat UMETA(DisplayName="Threat"),
    Rally UMETA(DisplayName="Rally"),
    Friendly UMETA(DisplayName="Friendly"),
    Extraction UMETA(DisplayName="Extraction"),
    Observation UMETA(DisplayName="Observation")
};

UENUM(BlueprintType)
enum class ETUMX50VideoFeedType : uint8
{
    FPVDrone UMETA(DisplayName="FPV Drone"),
    BodyCamera UMETA(DisplayName="Body Camera"),
    FixedCamera UMETA(DisplayName="Fixed Camera"),
    Observation UMETA(DisplayName="Observation Feed")
};

USTRUCT(BlueprintType)
struct FTMX50MapMarker
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MarkerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Label = FText::FromString(TEXT("Marker"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETUMX50MapMarkerType Type = ETUMX50MapMarkerType::Objective;

    /** Normalized 0..1 coordinates within the mission map/floor plan. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D NormalizedPosition = FVector2D(0.5f, 0.5f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Floor = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Details;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;
};

USTRUCT(BlueprintType)
struct FTMX50VideoFeed
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FeedId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName = FText::FromString(TEXT("Video Feed"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETUMX50VideoFeedType Type = ETUMX50VideoFeedType::Observation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText StatusText = FText::FromString(TEXT("Offline"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAvailable = false;
};

USTRUCT(BlueprintType)
struct FTMX50MissionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MissionId = TEXT("OP_KILLHOUSE");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText MissionTitle = FText::FromString(TEXT("Kill House Evaluation"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Objective = FText::FromString(TEXT("Clear the training structure and complete the assigned evaluation."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Area = FText::FromString(TEXT("Command Training Facility"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ThreatSummary = FText::FromString(TEXT("Training threat package. Live mission intelligence plugs into this field later."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText TeamSummary = FText::FromString(TEXT("Local operator. Co-op roster and readiness integrate here later."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDroneFeedAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDeploymentAuthorized = false;
};

/** Persistent state for the operator-worn MX50 tactical tablet. */
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUMX50TabletComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUMX50TabletComponent();

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetMissionContext(FName MissionId, const FText& MissionTitle);

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetMissionSnapshot(const FTMX50MissionSnapshot& InSnapshot);

    UFUNCTION(BlueprintCallable, Category="MX50|Mission")
    bool ApplyMissionPackage(const UTUMissionPackageData* MissionPackage);

    UFUNCTION(BlueprintPure, Category="MX50")
    FTMX50MissionSnapshot GetMissionSnapshot() const { return MissionSnapshot; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetActivePage(ETUMX50Page Page) { ActivePage = Page; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    ETUMX50Page CyclePage(int32 Direction = 1);

    UFUNCTION(BlueprintPure, Category="MX50")
    ETUMX50Page GetActivePage() const { return ActivePage; }

    UFUNCTION(BlueprintCallable, Category="MX50|Map")
    void SetMapMarkers(const TArray<FTMX50MapMarker>& InMarkers);

    UFUNCTION(BlueprintCallable, Category="MX50|Map")
    bool UpsertMapMarker(const FTMX50MapMarker& Marker);

    UFUNCTION(BlueprintCallable, Category="MX50|Map")
    bool RemoveMapMarker(FName MarkerId);

    UFUNCTION(BlueprintCallable, Category="MX50|Map")
    bool SelectMapMarker(FName MarkerId);

    UFUNCTION(BlueprintCallable, Category="MX50|Map")
    bool GetSelectedMapMarker(FTMX50MapMarker& OutMarker) const;

    UFUNCTION(BlueprintPure, Category="MX50|Map")
    TArray<FTMX50MapMarker> GetMapMarkers() const { return MapMarkers; }

    UFUNCTION(BlueprintPure, Category="MX50|Map")
    FName GetSelectedMapMarkerId() const { return SelectedMapMarkerId; }

    UFUNCTION(BlueprintCallable, Category="MX50|Map")
    void SetPlannedRoute(const TArray<FVector2D>& InRoutePoints);

    UFUNCTION(BlueprintPure, Category="MX50|Map")
    TArray<FVector2D> GetPlannedRoute() const { return PlannedRoute; }

    UFUNCTION(BlueprintCallable, Category="MX50|Video")
    void SetVideoFeeds(const TArray<FTMX50VideoFeed>& InFeeds);

    UFUNCTION(BlueprintCallable, Category="MX50|Video")
    bool UpsertVideoFeed(const FTMX50VideoFeed& Feed);

    UFUNCTION(BlueprintCallable, Category="MX50|Video")
    bool SelectVideoFeed(FName FeedId);

    UFUNCTION(BlueprintCallable, Category="MX50|Video")
    bool GetSelectedVideoFeed(FTMX50VideoFeed& OutFeed) const;

    UFUNCTION(BlueprintPure, Category="MX50|Video")
    TArray<FTMX50VideoFeed> GetVideoFeeds() const { return VideoFeeds; }

    UFUNCTION(BlueprintPure, Category="MX50|Video")
    FName GetSelectedVideoFeedId() const { return SelectedVideoFeedId; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetDroneFeedAvailable(bool bAvailable) { MissionSnapshot.bDroneFeedAvailable = bAvailable; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetDeploymentAuthorized(bool bAuthorized) { MissionSnapshot.bDeploymentAuthorized = bAuthorized; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MX50")
    FTMX50MissionSnapshot MissionSnapshot;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MX50")
    ETUMX50Page ActivePage = ETUMX50Page::Mission;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MX50|Map")
    TArray<FTMX50MapMarker> MapMarkers;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MX50|Map")
    TArray<FVector2D> PlannedRoute;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MX50|Map")
    FName SelectedMapMarkerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MX50|Video")
    TArray<FTMX50VideoFeed> VideoFeeds;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MX50|Video")
    FName SelectedVideoFeedId = NAME_None;
};
