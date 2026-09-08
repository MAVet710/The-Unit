#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUMX50ControllerWiringTest, "TheUnit.MX50.ControllerWiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUMX50ControllerWiringTest::RunTest(const FString& Parameters)
{
    const ATU_GameMode* GameModeCDO = GetDefault<ATU_GameMode>();
    if (!TestNotNull(TEXT("The Unit GameMode CDO"), GameModeCDO))
    {
        return false;
    }

    TestEqual(
        TEXT("Native GameMode uses the tactical PlayerController"),
        GameModeCDO->PlayerControllerClass,
        ATU_PlayerController::StaticClass());

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
