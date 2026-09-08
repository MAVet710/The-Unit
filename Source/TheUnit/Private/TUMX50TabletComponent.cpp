#include "TUMX50TabletComponent.h"

#include "TUMissionPackageData.h"

UTUMX50TabletComponent::UTUMX50TabletComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTUMX50TabletComponent::SetMissionContext(FName MissionId, const FText& MissionTitle)
{
    if (!MissionId.IsNone())
    {
        MissionSnapshot.MissionId = MissionId;
    }
    if (!MissionTitle.IsEmpty())
    {
        MissionSnapshot.MissionTitle = MissionTitle;
    }
}

void UTUMX50TabletComponent::SetMissionSnapshot(const FTMX50MissionSnapshot& InSnapshot)
{
    MissionSnapshot = InSnapshot;
}

bool UTUMX50TabletComponent::ApplyMissionPackage(const UTUMissionPackageData* MissionPackage)
{
    if (!MissionPackage)
    {
        return false;
    }

    MissionSnapshot = MissionPackage->Mission;
    SetMapMarkers(MissionPackage->MapMarkers);
    SetPlannedRoute(MissionPackage->PlannedRoute);
    SetVideoFeeds(MissionPackage->VideoFeeds);
    ActivePage = ETUMX50Page::Mission;
    return true;
}

ETUMX50Page UTUMX50TabletComponent::CyclePage(int32 Direction)
{
    constexpr int32 PageCount = static_cast<int32>(ETUMX50Page::Loadout) + 1;
    int32 Index = static_cast<int32>(ActivePage);
    const int32 Step = Direction < 0 ? -1 : 1;
    Index = (Index + Step + PageCount) % PageCount;
    ActivePage = static_cast<ETUMX50Page>(Index);
    return ActivePage;
}

void UTUMX50TabletComponent::RefreshMapSelection()
{
    for (const FTMX50MapMarker& Marker : MapMarkers)
    {
        if (Marker.MarkerId == SelectedMapMarkerId && Marker.bVisible)
        {
            return;
        }
    }

    SelectedMapMarkerId = NAME_None;
    for (const FTMX50MapMarker& Marker : MapMarkers)
    {
        if (Marker.bVisible && !Marker.MarkerId.IsNone())
        {
            SelectedMapMarkerId = Marker.MarkerId;
            return;
        }
    }
}

void UTUMX50TabletComponent::SetMapMarkers(const TArray<FTMX50MapMarker>& InMarkers)
{
    MapMarkers = InMarkers;
    for (FTMX50MapMarker& Marker : MapMarkers)
    {
        Marker.NormalizedPosition.X = FMath::Clamp(Marker.NormalizedPosition.X, 0.0, 1.0);
        Marker.NormalizedPosition.Y = FMath::Clamp(Marker.NormalizedPosition.Y, 0.0, 1.0);
    }
    SelectedMapMarkerId = NAME_None;
    RefreshMapSelection();
}

bool UTUMX50TabletComponent::UpsertMapMarker(const FTMX50MapMarker& Marker)
{
    if (Marker.MarkerId.IsNone())
    {
        return false;
    }

    FTMX50MapMarker Sanitized = Marker;
    Sanitized.NormalizedPosition.X = FMath::Clamp(Sanitized.NormalizedPosition.X, 0.0, 1.0);
    Sanitized.NormalizedPosition.Y = FMath::Clamp(Sanitized.NormalizedPosition.Y, 0.0, 1.0);

    bool bUpdated = false;
    for (FTMX50MapMarker& Existing : MapMarkers)
    {
        if (Existing.MarkerId == Sanitized.MarkerId)
        {
            Existing = Sanitized;
            bUpdated = true;
            break;
        }
    }

    if (!bUpdated)
    {
        MapMarkers.Add(Sanitized);
    }

    RefreshMapSelection();
    return true;
}

bool UTUMX50TabletComponent::RemoveMapMarker(FName MarkerId)
{
    if (MarkerId.IsNone())
    {
        return false;
    }

    const int32 Removed = MapMarkers.RemoveAll([MarkerId](const FTMX50MapMarker& Marker)
    {
        return Marker.MarkerId == MarkerId;
    });

    if (Removed <= 0)
    {
        return false;
    }

    RefreshMapSelection();
    return true;
}

bool UTUMX50TabletComponent::SelectMapMarker(FName MarkerId)
{
    for (const FTMX50MapMarker& Marker : MapMarkers)
    {
        if (Marker.MarkerId == MarkerId && Marker.bVisible)
        {
            SelectedMapMarkerId = MarkerId;
            return true;
        }
    }
    return false;
}

bool UTUMX50TabletComponent::GetSelectedMapMarker(FTMX50MapMarker& OutMarker) const
{
    for (const FTMX50MapMarker& Marker : MapMarkers)
    {
        if (Marker.MarkerId == SelectedMapMarkerId && Marker.bVisible)
        {
            OutMarker = Marker;
            return true;
        }
    }
    return false;
}

void UTUMX50TabletComponent::SetPlannedRoute(const TArray<FVector2D>& InRoutePoints)
{
    PlannedRoute.Reset(InRoutePoints.Num());
    for (FVector2D Point : InRoutePoints)
    {
        Point.X = FMath::Clamp(Point.X, 0.0, 1.0);
        Point.Y = FMath::Clamp(Point.Y, 0.0, 1.0);
        PlannedRoute.Add(Point);
    }
}

void UTUMX50TabletComponent::RefreshVideoDerivedState()
{
    MissionSnapshot.bDroneFeedAvailable = false;

    bool bSelectedStillAvailable = false;
    for (const FTMX50VideoFeed& Feed : VideoFeeds)
    {
        if (Feed.bAvailable && Feed.FeedId == SelectedVideoFeedId)
        {
            bSelectedStillAvailable = true;
        }
        if (Feed.bAvailable && Feed.Type == ETUMX50VideoFeedType::FPVDrone)
        {
            MissionSnapshot.bDroneFeedAvailable = true;
        }
    }

    if (bSelectedStillAvailable)
    {
        return;
    }

    SelectedVideoFeedId = NAME_None;
    for (const FTMX50VideoFeed& Feed : VideoFeeds)
    {
        if (Feed.bAvailable && !Feed.FeedId.IsNone())
        {
            SelectedVideoFeedId = Feed.FeedId;
            return;
        }
    }
}

void UTUMX50TabletComponent::SetVideoFeeds(const TArray<FTMX50VideoFeed>& InFeeds)
{
    VideoFeeds = InFeeds;
    SelectedVideoFeedId = NAME_None;
    RefreshVideoDerivedState();
}

bool UTUMX50TabletComponent::UpsertVideoFeed(const FTMX50VideoFeed& Feed)
{
    if (Feed.FeedId.IsNone())
    {
        return false;
    }

    bool bUpdated = false;
    for (FTMX50VideoFeed& Existing : VideoFeeds)
    {
        if (Existing.FeedId == Feed.FeedId)
        {
            Existing = Feed;
            bUpdated = true;
            break;
        }
    }

    if (!bUpdated)
    {
        VideoFeeds.Add(Feed);
    }

    RefreshVideoDerivedState();
    return true;
}

bool UTUMX50TabletComponent::RemoveVideoFeed(FName FeedId)
{
    if (FeedId.IsNone())
    {
        return false;
    }

    const int32 Removed = VideoFeeds.RemoveAll([FeedId](const FTMX50VideoFeed& Feed)
    {
        return Feed.FeedId == FeedId;
    });

    if (Removed <= 0)
    {
        return false;
    }

    RefreshVideoDerivedState();
    return true;
}

bool UTUMX50TabletComponent::SelectVideoFeed(FName FeedId)
{
    for (const FTMX50VideoFeed& Feed : VideoFeeds)
    {
        if (Feed.FeedId == FeedId && Feed.bAvailable)
        {
            SelectedVideoFeedId = FeedId;
            return true;
        }
    }
    return false;
}

bool UTUMX50TabletComponent::GetSelectedVideoFeed(FTMX50VideoFeed& OutFeed) const
{
    for (const FTMX50VideoFeed& Feed : VideoFeeds)
    {
        if (Feed.FeedId == SelectedVideoFeedId && Feed.bAvailable)
        {
            OutFeed = Feed;
            return true;
        }
    }
    return false;
}
