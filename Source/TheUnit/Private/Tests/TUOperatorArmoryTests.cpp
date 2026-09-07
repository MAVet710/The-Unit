#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_AK105.h"
#include "TU_G34CM.h"
#include "TU_M110.h"
#include "TU_RGRFive7.h"
#include "TU_TacticalRifle.h"
#include "TUOperatorLoadoutComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorLoadoutDataTest, "TheUnit.Combat.OperatorLoadout.Data",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorLoadoutDataTest::RunTest(const FString& Parameters)
{
    UTUOperatorLoadoutComponent* Loadout = NewObject<UTUOperatorLoadoutComponent>();
    if (!TestNotNull(TEXT("Operator loadout component"), Loadout))
    {
        return false;
    }

    TestEqual(TEXT("Three built-in primary choices"), Loadout->GetPrimaryItems().Num(), 3);
    TestEqual(TEXT("Two built-in secondary choices"), Loadout->GetSecondaryItems().Num(), 2);
    TestEqual(TEXT("Equipment boundary begins with one empty choice"), Loadout->GetEquipmentItems().Num(), 1);
    TestEqual(TEXT("TU-556 selected as default primary"), Loadout->GetSelectedPrimaryId(), FName(TEXT("PRIMARY_TU556")));
    TestEqual(TEXT("G34 CM selected as default secondary"), Loadout->GetSelectedSecondaryId(), FName(TEXT("SECONDARY_G34CM")));
    TestTrue(TEXT("Default primary class is TU-556"), Loadout->GetSelectedPrimaryClass() == ATU_TacticalRifle::StaticClass());
    TestTrue(TEXT("Default secondary class is G34 CM"), Loadout->GetSelectedSecondaryClass() == ATU_G34CM::StaticClass());

    TestTrue(TEXT("AK-105 can be selected"), Loadout->SelectPrimaryById(TEXT("PRIMARY_AK105")));
    TestTrue(TEXT("AK-105 class becomes selected"), Loadout->GetSelectedPrimaryClass() == ATU_AK105::StaticClass());
    TestTrue(TEXT("RGR Five7 can be selected"), Loadout->SelectSecondaryById(TEXT("SECONDARY_RGRFIVE7")));
    TestTrue(TEXT("RGR Five7 class becomes selected"), Loadout->GetSelectedSecondaryClass() == ATU_RGRFive7::StaticClass());
    TestFalse(TEXT("Unknown primary id is rejected"), Loadout->SelectPrimaryById(TEXT("PRIMARY_MISSING")));
    TestFalse(TEXT("Unknown secondary id is rejected"), Loadout->SelectSecondaryById(TEXT("SECONDARY_MISSING")));
    TestTrue(TEXT("Selected non-melee weight is non-negative"), Loadout->GetSelectedNonMeleeWeightKg() >= 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorWeaponSlotRuntimeTest, "TheUnit.Combat.OperatorLoadout.RuntimeSlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorWeaponSlotRuntimeTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        true, ERHIFeatureLevel::Num, &InitValues);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    ATU_ArmedOperatorCharacter* Operator = World->SpawnActor<ATU_ArmedOperatorCharacter>();
    if (!TestNotNull(TEXT("Armed operator"), Operator))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestNotNull(TEXT("Operator owns primary/secondary loadout component"), Operator->GetOperatorLoadout());
    TestTrue(TEXT("Operator can spawn both firearm slots"), Operator->SpawnDefaultWeapon());
    TestNotNull(TEXT("Primary runtime actor exists"), Operator->GetPrimaryWeapon());
    TestNotNull(TEXT("Secondary runtime actor exists"), Operator->GetSecondaryWeapon());
    TestTrue(TEXT("Primary starts active"), Operator->GetCurrentWeapon() == Operator->GetPrimaryWeapon());
    TestEqual(TEXT("Primary active slot enum"), Operator->GetActiveWeaponSlot(), ETUOperatorWeaponSlot::Primary);

    ATU_WeaponBase* OriginalPrimary = Operator->GetPrimaryWeapon();
    ATU_WeaponBase* OriginalSecondary = Operator->GetSecondaryWeapon();
    TestTrue(TEXT("Secondary slot can be equipped"), Operator->EquipWeaponSlot(ETUOperatorWeaponSlot::Secondary));
    TestTrue(TEXT("Current weapon becomes secondary"), Operator->GetCurrentWeapon() == Operator->GetSecondaryWeapon());
    TestTrue(TEXT("Primary actor remains alive across slot switch"), Operator->GetPrimaryWeapon() == OriginalPrimary);

    TestTrue(TEXT("Secondary selection can change to RGR Five7"), Operator->SelectSecondaryById(TEXT("SECONDARY_RGRFIVE7")));
    TestTrue(TEXT("Secondary runtime actor was replaced"), Operator->GetSecondaryWeapon() != OriginalSecondary);
    TestTrue(TEXT("New secondary runtime class is RGR Five7"), Operator->GetSecondaryWeapon() && Operator->GetSecondaryWeapon()->IsA<ATU_RGRFive7>());
    TestTrue(TEXT("Primary actor was not replaced by secondary selection"), Operator->GetPrimaryWeapon() == OriginalPrimary);
    TestTrue(TEXT("Active weapon follows replaced secondary"), Operator->GetCurrentWeapon() == Operator->GetSecondaryWeapon());

    TestTrue(TEXT("Primary selection can change to M110"), Operator->SelectPrimaryById(TEXT("PRIMARY_M110")));
    TestTrue(TEXT("Selected primary runtime class is M110"), Operator->GetPrimaryWeapon() && Operator->GetPrimaryWeapon()->IsA<ATU_M110>());
    TestTrue(TEXT("Primary can be re-equipped"), Operator->EquipWeaponSlot(ETUOperatorWeaponSlot::Primary));
    TestTrue(TEXT("Current weapon becomes new M110 primary"), Operator->GetCurrentWeapon() == Operator->GetPrimaryWeapon());
    TestTrue(TEXT("Combined selected loadout weight includes carried slots"), Operator->GetSelectedLoadoutWeightKg() > 0.0f);

    World->DestroyWorld(false);
    return true;
}

#endif
