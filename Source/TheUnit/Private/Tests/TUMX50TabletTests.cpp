#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUMissionPackageData.h"
#include "TUMX50TabletComponent.h"
#include "TU_GameMode.h"
#include "TU_PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUMX50TabletStateTest, "TheUnit.MX50.StateAndPages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUMX50TabletStateTest::RunTest(const FString& Parameters)
{
    UTUMX50TabletComponent* Tablet = NewObject<UTUMX50TabletComponent>();
    if (!TestNotNull(TEXT("MX50 tablet component"), Tablet))
    {
        return false;
    }

    TestEqual(TEXT("MX50 defaults to Mission page"), Tablet->GetActivePage(), ETUMX50Page::Mission);
    TestEqual(TEXT("Default operation id"), Tablet->GetMissionSnapshot().MissionId, FName(TEXT("OP_KILLHOUSE")));
    TestFalse(TEXT("Drone feed is unavailable by default"), Tablet->GetMissionSnapshot().bDroneFeedAvailable);
    TestFalse(TEXT("Deployment is not automatically authorized"), Tablet->GetMissionSnapshot().bDeploymentAuthorized);

    Tablet->SetMissionContext(TEXT("OP_TEST_ALPHA"), FText::FromString(TEXT("Operation Test Alpha")));
    TestEqual(TEXT("Mission context id updates"), Tablet->GetMissionSnapshot().MissionId, FName(TEXT("OP_TEST_ALPHA")));
    TestEqual(TEXT("Mission context title updates"), Tablet->GetMissionSnapshot().MissionTitle.ToString(), FString(TEXT("Operation Test Alpha")));

    TestEqual(TEXT("Forward page cycle reaches Map"), Tablet->CyclePage(1), ETUMX50Page::Map);
    Tablet->SetActivePage(ETUMX50Page::Loadout);
    TestEqual(TEXT("Forward cycle wraps Loadout to Mission"), Tablet->CyclePage(1), ETUMX50Page::Mission);
    TestEqual(TEXT("Backward cycle wraps Mission to Loadout"), Tablet->CyclePage(-1), ETUMX50Page::Loadout);

    Tablet->SetDroneFeedAvailable(true);
    Tablet->SetDeploymentAuthorized(true);
    TestTrue(TEXT("Drone availability can be updated independently"), Tablet->GetMissionSnapshot().bDroneFeedAvailable);
    TestTrue(TEXT("Deployment authorization can be updated independently"), Tablet->GetMissionSnapshot().bDeploymentAuthorized);

    FTMX50MissionSnapshot Snapshot;
    Snapshot.MissionId = TEXT("OP_REPLACED");
    Snapshot.MissionTitle = FText::FromString(TEXT("Replacement Snapshot"));
    Snapshot.Area = FText::FromString(TEXT("Test AO"));
    Snapshot.bDroneFeedAvailable = false;
    Tablet->SetMissionSnapshot(Snapshot);

    TestEqual(TEXT("Full snapshot replacement updates mission id"), Tablet->GetMissionSnapshot().MissionId, FName(TEXT("OP_REPLACED")));
    TestEqual(TEXT("Full snapshot replacement updates area"), Tablet->GetMissionSnapshot().Area.ToString(), FString(TEXT("Test AO")));
    TestFalse(TEXT("Full snapshot replacement updates feed state"), Tablet->GetMissionSnapshot().bDroneFeedAvailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUMX50MissionPackageTest, "TheUnit.MX50.MissionPackageMapVideo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUMX50MissionPackageTest::RunTest(const FString& Parameters)
{
    UTUMX50TabletComponent* Tablet = NewObject<UTUMX50TabletComponent>();
    UTUMissionPackageData* Package = NewObject<UTUMissionPackageData>();
    if (!TestNotNull(TEXT("Tablet"), Tablet) || !TestNotNull(TEXT("Mission package"), Package))
    {
        return false;
    }

    Package->Mission.MissionId = TEXT("OP_PACKAGE_TEST");
    Package->Mission.MissionTitle = FText::FromString(TEXT("Package Test"));
    Package->Mission.Area = FText::FromString(TEXT("Test Structure"));

    FTMX50MapMarker Entry;
    Entry.MarkerId = TEXT("ENTRY_ALPHA");
    Entry.Label = FText::FromString(TEXT("Alpha Entry"));
    Entry.Type = ETUMX50MapMarkerType::Entry;
    Entry.NormalizedPosition = FVector2D(-0.25, 1.4);
    Entry.Details = FText::FromString(TEXT("Primary entry point"));
    Package->MapMarkers.Add(Entry);

    FTMX50MapMarker Objective;
    Objective.MarkerId = TEXT("OBJ_ONE");
    Objective.Label = FText::FromString(TEXT("Objective One"));
    Objective.Type = ETUMX50MapMarkerType::Objective;
    Objective.NormalizedPosition = FVector2D(0.55, 0.35);
    Package->MapMarkers.Add(Objective);

    Package->PlannedRoute = { FVector2D(-1.0, 0.2), FVector2D(0.5, 0.5), FVector2D(1.5, 0.8) };

    FTMX50VideoFeed Drone;
    Drone.FeedId = TEXT("FPV_ALPHA");
    Drone.DisplayName = FText::FromString(TEXT("FPV Alpha"));
    Drone.Type = ETUMX50VideoFeedType::FPVDrone;
    Drone.StatusText = FText::FromString(TEXT("Link Ready"));
    Drone.bAvailable = true;
    Package->VideoFeeds.Add(Drone);

    FTMX50VideoFeed Camera;
    Camera.FeedId = TEXT("CAM_ONE");
    Camera.DisplayName = FText::FromString(TEXT("Hallway Camera"));
    Camera.Type = ETUMX50VideoFeedType::FixedCamera;
    Camera.StatusText = FText::FromString(TEXT("Offline"));
    Camera.bAvailable = false;
    Package->VideoFeeds.Add(Camera);

    TestTrue(TEXT("Mission package applies"), Tablet->ApplyMissionPackage(Package));
    TestEqual(TEXT("Mission id comes from package"), Tablet->GetMissionSnapshot().MissionId, FName(TEXT("OP_PACKAGE_TEST")));
    TestEqual(TEXT("Package resets tablet to Mission page"), Tablet->GetActivePage(), ETUMX50Page::Mission);
    TestTrue(TEXT("Available FPV feed sets drone availability"), Tablet->GetMissionSnapshot().bDroneFeedAvailable);

    const TArray<FTMX50MapMarker> Markers = Tablet->GetMapMarkers();
    TestEqual(TEXT("Two map markers loaded"), Markers.Num(), 2);
    TestEqual(TEXT("Marker X clamps low"), Markers[0].NormalizedPosition.X, 0.0);
    TestEqual(TEXT("Marker Y clamps high"), Markers[0].NormalizedPosition.Y, 1.0);
    TestEqual(TEXT("First visible marker auto-selects"), Tablet->GetSelectedMapMarkerId(), FName(TEXT("ENTRY_ALPHA")));

    TestTrue(TEXT("Objective marker can be selected"), Tablet->SelectMapMarker(TEXT("OBJ_ONE")));
    FTMX50MapMarker SelectedMarker;
    TestTrue(TEXT("Selected marker resolves"), Tablet->GetSelectedMapMarker(SelectedMarker));
    TestEqual(TEXT("Selected marker id is objective"), SelectedMarker.MarkerId, FName(TEXT("OBJ_ONE")));

    TestTrue(TEXT("Selected marker can be removed"), Tablet->RemoveMapMarker(TEXT("OBJ_ONE")));
    TestFalse(TEXT("Removed marker cannot be selected"), Tablet->SelectMapMarker(TEXT("OBJ_ONE")));

    const TArray<FVector2D> Route = Tablet->GetPlannedRoute();
    TestEqual(TEXT("Three route points loaded"), Route.Num(), 3);
    TestEqual(TEXT("Route start clamps low"), Route[0].X, 0.0);
    TestEqual(TEXT("Route end clamps high"), Route[2].X, 1.0);

    TestEqual(TEXT("Available FPV feed auto-selects"), Tablet->GetSelectedVideoFeedId(), FName(TEXT("FPV_ALPHA")));
    FTMX50VideoFeed SelectedFeed;
    TestTrue(TEXT("Selected feed resolves"), Tablet->GetSelectedVideoFeed(SelectedFeed));
    TestEqual(TEXT("Selected feed is FPV"), SelectedFeed.Type, ETUMX50VideoFeedType::FPVDrone);
    TestFalse(TEXT("Offline feed cannot be selected"), Tablet->SelectVideoFeed(TEXT("CAM_ONE")));

    FTMX50MapMarker Rally;
    Rally.MarkerId = TEXT("RALLY_ONE");
    Rally.Type = ETUMX50MapMarkerType::Rally;
    Rally.NormalizedPosition = FVector2D(0.25, 0.75);
    TestTrue(TEXT("Runtime map marker can be added"), Tablet->UpsertMapMarker(Rally));
    TestEqual(TEXT("Map marker count returns to two"), Tablet->GetMapMarkers().Num(), 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUMX50ControllerWiringTest, "TheUnit.MX50.ControllerWiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUMX50ControllerWiringTest::RunTest(const FString& Parameters)
{
    const ATU_GameMode* GameModeCDO = GetDefault<ATU_GameMode>();
    if (!TestNotNull(TEXT("The Unit GameMode CDO"), GameModeCDO))
    {
        return false;
    }

    TestTrue(
        TEXT("Native GameMode uses the tactical PlayerController"),
        GameModeCDO->PlayerControllerClass == ATU_PlayerController::StaticClass());

    const ATU_PlayerController* ControllerCDO = GetDefault<ATU_PlayerController>();
    if (!TestNotNull(TEXT("Tactical PlayerController CDO"), ControllerCDO))
    {
        return false;
    }

    UTUMX50TabletComponent* PersistentTablet = ControllerCDO->GetMX50Tablet();
    if (!TestNotNull(TEXT("PlayerController owns persistent MX50 state"), PersistentTablet))
    {
        return false;
    }

    TestEqual(TEXT("Controller MX50 begins on Mission page"), PersistentTablet->GetActivePage(), ETUMX50Page::Mission);
    TestEqual(TEXT("Controller MX50 has default mission context"), PersistentTablet->GetMissionSnapshot().MissionId, FName(TEXT("OP_KILLHOUSE")));
    return true;
}

#endif
