#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TUMX50TabletComponent.h"
#include "TUMissionPackageData.generated.h"

/** Data-authored operation package consumed by the operator-worn MX50. */
UCLASS(BlueprintType)
class THEUNIT_API UTUMissionPackageData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
    FTMX50MissionSnapshot Mission;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    TArray<FTMX50MapMarker> MapMarkers;

    /** Normalized 0..1 route points drawn over the selected mission map/floor plan. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    TArray<FVector2D> PlannedRoute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Video")
    TArray<FTMX50VideoFeed> VideoFeeds;
};
