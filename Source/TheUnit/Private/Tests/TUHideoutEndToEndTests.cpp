#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUEquipmentDefinition.h"
#include "TUHideoutSaveGame.h"
#include "TUMissionPackageData.h"
#include "TUArmorProtectionComponent.h"
#include "TUOperatorEquipmentComponent.h"
#include "TU_GameMode.h"
#include "TU_HideoutGameMode.h"
#include "TU_HideoutUpgradeStation.h"
#include "TU_ModularOperatorCharacter.h"
#include "TU_TrainingMissionGameMode.h"
#include "TU_ExtractionZone.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUIntegratedOperatorWiringTest, "TheUnit.Hideout.EndToEnd.IntegratedOperator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUIntegratedOperatorWiringTest::RunTest(const FString& Parameters)
{
    const ATU_GameMode* GameModeCDO = GetDefault<ATU_GameMode>();
    if (!TestNotNull(TEXT("The Unit GameMode CDO"), GameModeCDO))
    {
        return false;
    }

    TestTrue(TEXT("Default pawn is the combined modular armed operator"),
        GameModeCDO->DefaultPawnClass == ATU_ModularOperatorCharacter::StaticClass());

    const ATU_ModularOperatorCharacter* OperatorCDO = GetDefault<ATU_ModularOperatorCharacter>();
    if (!TestNotNull(TEXT("Integrated operator CDO"), OperatorCDO))
    {
        return false;
    }

    TestNotNull(TEXT("Integrated operator owns modular equipment"), OperatorCDO->GetOperatorEquipment());
    TestNotNull(TEXT("Integrated operator owns armor resolver"), OperatorCDO->GetArmorProtection());
    TestNotNull(TEXT("Integrated operator owns regional health"), OperatorCDO->GetOperatorHealth());
    TestNotNull(TEXT("Integrated operator retains primary/secondary loadout component"), OperatorCDO->GetOperatorLoadout());
    TestNotNull(TEXT("Integrated operator retains selectable melee component"), OperatorCDO->GetMeleeLoadout());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHideoutSaveSchemaTest, "TheUnit.Hideout.EndToEnd.SaveSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUHideoutSaveSchemaTest::RunTest(const FString& Parameters)
{
    UTUHideoutSaveGame* Save = NewObject<UTUHideoutSaveGame>();
    if (!TestNotNull(TEXT("Hideout save object"), Save))
    {
        return false;
    }

    TestEqual(TEXT("Current save schema version"), Save->SaveVersion, 2);
    TestTrue(TEXT("New profile has no persisted tactical gear overrides"), Save->GearBySlot.IsEmpty());
    TestFalse(TEXT("New profile does not begin inside a mission"), Save->bMissionInProgress);
    TestEqual(TEXT("Default return map token"), Save->HideoutMapName, FName(TEXT("CommandCenter")));

    Save->PrimaryId = TEXT("WPN_TU556");
    Save->MeleeId = TEXT("MELEE_OTF");
    Save->GearBySlot.Add(ETUEquipmentSlot::Headwear, TEXT("EQ_BallisticHelmet"));
    Save->GearBySlot.Add(ETUEquipmentSlot::ChestRig, NAME_None);

    TestEqual(TEXT("Primary selection is representable"), Save->PrimaryId, FName(TEXT("WPN_TU556")));
    TestEqual(TEXT("Helmet slot stores a stable item id"), Save->GearBySlot.FindRef(ETUEquipmentSlot::Headwear), FName(TEXT("EQ_BallisticHelmet")));
    TestTrue(TEXT("Explicit empty gear slots are representable"), Save->GearBySlot.Contains(ETUEquipmentSlot::ChestRig));
    TestTrue(TEXT("Explicit empty gear slot stores NAME_None"), Save->GearBySlot.FindRef(ETUEquipmentSlot::ChestRig).IsNone());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUMissionTravelDataTest, "TheUnit.Hideout.EndToEnd.MissionTravelData",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUMissionTravelDataTest::RunTest(const FString& Parameters)
{
    UTUMissionPackageData* Mission = NewObject<UTUMissionPackageData>();
    if (!TestNotNull(TEXT("Mission package"), Mission))
    {
        return false;
    }

    TestTrue(TEXT("Mission destination is author-required rather than fabricated"), Mission->DestinationMap.IsNone());
    TestFalse(TEXT("Deployment starts unauthorized"), Mission->Mission.bDeploymentAuthorized);

    Mission->Mission.MissionId = TEXT("OP_E2E_TEST");
    Mission->DestinationMap = TEXT("Killhouse");
    Mission->Mission.bDeploymentAuthorized = true;

    TestEqual(TEXT("Mission id can drive persistence"), Mission->Mission.MissionId, FName(TEXT("OP_E2E_TEST")));
    TestEqual(TEXT("Destination map can drive travel"), Mission->DestinationMap, FName(TEXT("Killhouse")));
    TestTrue(TEXT("Deployment authorization is explicit"), Mission->Mission.bDeploymentAuthorized);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHideoutPhysicalLifecycleTest, "TheUnit.Hideout.EndToEnd.PhysicalLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUHideoutPhysicalLifecycleTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Hideout bootstrap mode inherits The Unit GameMode"),
        ATU_HideoutGameMode::StaticClass()->IsChildOf(ATU_GameMode::StaticClass()));
    TestTrue(TEXT("Training mission bootstrap mode inherits The Unit GameMode"),
        ATU_TrainingMissionGameMode::StaticClass()->IsChildOf(ATU_GameMode::StaticClass()));
    TestTrue(TEXT("Upgrade station remains a normal physical command-center station"),
        ATU_HideoutUpgradeStation::StaticClass()->IsChildOf(ATU_CommandCenterStation::StaticClass()));

    const ATU_HideoutUpgradeStation* UpgradeCDO = GetDefault<ATU_HideoutUpgradeStation>();
    TestNotNull(TEXT("Upgrade station CDO"), UpgradeCDO);
    if (UpgradeCDO)
    {
        TestEqual(TEXT("Upgrade station has deterministic default module"), UpgradeCDO->GetModuleType(), ETUHideoutModuleType::Power);
    }

    const ATU_ExtractionZone* ExtractionCDO = GetDefault<ATU_ExtractionZone>();
    TestNotNull(TEXT("Extraction zone CDO"), ExtractionCDO);
    if (ExtractionCDO)
    {
        TestFalse(TEXT("Extraction is not pending on spawn"), ExtractionCDO->IsExtractionPending());
    }
    return true;
}

#endif
