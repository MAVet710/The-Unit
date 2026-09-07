#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUEquipmentDefinition.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorEquipmentStateTest, "TheUnit.Operator.Equipment.State",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorEquipmentStateTest::RunTest(const FString& Parameters)
{
    UTUOperatorEquipmentComponent* Equipment = NewObject<UTUOperatorEquipmentComponent>();
    if (!TestNotNull(TEXT("Equipment component"), Equipment))
    {
        return false;
    }

    UTUEquipmentDefinition* HelmetA = NewObject<UTUEquipmentDefinition>();
    HelmetA->ItemId = TEXT("helmet_a");
    HelmetA->Slot = ETUEquipmentSlot::Headwear;
    HelmetA->WeightKg = 1.20f;
    HelmetA->bProvidesBallisticProtection = true;
    HelmetA->ProtectedRegions.Add(ETUBodyRegion::Head);
    HelmetA->PenetrationResistance = 18.0f;
    HelmetA->MaxArmorDurability = 100.0f;
    HelmetA->StoppedRoundDamageMultiplier = 0.35f;
    HelmetA->RegionalCoverageFraction = 0.75f;

    UTUEquipmentDefinition* HelmetB = NewObject<UTUEquipmentDefinition>();
    HelmetB->ItemId = TEXT("helmet_b");
    HelmetB->Slot = ETUEquipmentSlot::Headwear;
    HelmetB->WeightKg = 0.80f;

    UTUEquipmentDefinition* Carrier = NewObject<UTUEquipmentDefinition>();
    Carrier->ItemId = TEXT("carrier_a");
    Carrier->Slot = ETUEquipmentSlot::TorsoArmor;
    Carrier->WeightKg = 6.50f;

    TestTrue(TEXT("Ballistic helmet marks head as protected"), HelmetA->ProtectedRegions.Contains(ETUBodyRegion::Head));
    TestTrue(TEXT("Ballistic resistance is authored independently of visuals"), FMath::IsNearlyEqual(HelmetA->PenetrationResistance, 18.0f));
    TestTrue(TEXT("Coverage remains bounded by authored value"), FMath::IsNearlyEqual(HelmetA->RegionalCoverageFraction, 0.75f));

    TestTrue(TEXT("Equip first helmet"), Equipment->EquipItem(HelmetA));
    TestEqual(TEXT("Head slot points to first helmet"), Equipment->GetEquippedItem(ETUEquipmentSlot::Headwear), HelmetA);
    TestEqual(TEXT("One item equipped"), Equipment->GetEquippedItemCount(), 1);
    TestTrue(TEXT("Weight includes first helmet"), FMath::IsNearlyEqual(Equipment->GetTotalEquipmentWeightKg(), 1.20f));

    TestTrue(TEXT("Replacing occupied slot succeeds"), Equipment->EquipItem(HelmetB));
    TestEqual(TEXT("Replacement owns head slot"), Equipment->GetEquippedItem(ETUEquipmentSlot::Headwear), HelmetB);
    TestEqual(TEXT("Replacement does not increase item count"), Equipment->GetEquippedItemCount(), 1);
    TestTrue(TEXT("Replacement updates total weight"), FMath::IsNearlyEqual(Equipment->GetTotalEquipmentWeightKg(), 0.80f));

    TestTrue(TEXT("Equip independent torso slot"), Equipment->EquipItem(Carrier));
    TestEqual(TEXT("Two slots occupied"), Equipment->GetEquippedItemCount(), 2);
    TestTrue(TEXT("Weight sums occupied slots"), FMath::IsNearlyEqual(Equipment->GetTotalEquipmentWeightKg(), 7.30f));

    TestTrue(TEXT("Unequip existing slot reports true"), Equipment->UnequipSlot(ETUEquipmentSlot::Headwear));
    TestFalse(TEXT("Head slot is empty"), Equipment->IsSlotOccupied(ETUEquipmentSlot::Headwear));
    TestFalse(TEXT("Unequip empty slot reports false"), Equipment->UnequipSlot(ETUEquipmentSlot::Headwear));

    UTUOperatorLoadoutData* Loadout = NewObject<UTUOperatorLoadoutData>();
    Loadout->Items.Add(HelmetA);
    Loadout->Items.Add(Carrier);

    TestTrue(TEXT("Apply data-driven loadout"), Equipment->ApplyLoadout(Loadout));
    TestEqual(TEXT("Loadout equips both slots"), Equipment->GetEquippedItemCount(), 2);
    TestEqual(TEXT("Loadout restores helmet A"), Equipment->GetEquippedItem(ETUEquipmentSlot::Headwear), HelmetA);
    TestEqual(TEXT("Loadout equips carrier"), Equipment->GetEquippedItem(ETUEquipmentSlot::TorsoArmor), Carrier);
    TestTrue(TEXT("Loadout weight is deterministic"), FMath::IsNearlyEqual(Equipment->GetTotalEquipmentWeightKg(), 7.70f));

    Equipment->ClearLoadout();
    TestEqual(TEXT("Clear removes all logical equipment"), Equipment->GetEquippedItemCount(), 0);
    TestTrue(TEXT("Clear resets weight"), FMath::IsNearlyZero(Equipment->GetTotalEquipmentWeightKg()));

    return true;
}

#endif
