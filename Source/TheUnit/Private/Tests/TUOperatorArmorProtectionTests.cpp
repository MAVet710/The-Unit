#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUArmorProtectionComponent.h"
#include "TUEquipmentDefinition.h"
#include "TUOperatorEquipmentComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorArmorProtectionTest, "TheUnit.Operator.Armor.BallisticProtection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorArmorProtectionTest::RunTest(const FString& Parameters)
{
    UTUOperatorEquipmentComponent* Equipment = NewObject<UTUOperatorEquipmentComponent>();
    UTUArmorProtectionComponent* Armor = NewObject<UTUArmorProtectionComponent>();

    if (!TestNotNull(TEXT("Equipment component"), Equipment) || !TestNotNull(TEXT("Armor component"), Armor))
    {
        return false;
    }

    Armor->InitializeEquipment(Equipment);

    UTUEquipmentDefinition* Helmet = NewObject<UTUEquipmentDefinition>();
    Helmet->ItemId = TEXT("ballistic_helmet_test");
    Helmet->Slot = ETUEquipmentSlot::Headwear;
    Helmet->bProvidesBallisticProtection = true;
    Helmet->ProtectedRegions.Add(ETUBodyRegion::Head);
    Helmet->PenetrationResistance = 15.0f;
    Helmet->MaxArmorDurability = 20.0f;
    Helmet->StoppedRoundDamageMultiplier = 0.25f;
    Helmet->RegionalCoverageFraction = 1.0f;

    TestTrue(TEXT("Equip ballistic helmet"), Equipment->EquipItem(Helmet));

    const FTUArmorHitResult Stopped = Armor->ResolveBallisticHit(
        ETUBodyRegion::Head,
        40.0f,
        10.0f,
        5.0f,
        0.5f);

    TestTrue(TEXT("Helmet is detected"), Stopped.bArmorPresent);
    TestTrue(TEXT("Helmet coverage is hit"), Stopped.bCoverageHit);
    TestTrue(TEXT("Low penetration hit is stopped"), Stopped.bStopped);
    TestFalse(TEXT("Stopped hit is not penetrated"), Stopped.bPenetrated);
    TestTrue(TEXT("Stopped hit applies residual damage"), FMath::IsNearlyEqual(Stopped.FinalDamage, 10.0f));
    TestTrue(TEXT("Armor durability loses ammo armor damage"), FMath::IsNearlyEqual(Stopped.DurabilityAfter, 15.0f));

    const FTUArmorHitResult Penetrated = Armor->ResolveBallisticHit(
        ETUBodyRegion::Head,
        40.0f,
        20.0f,
        5.0f,
        0.5f);

    TestTrue(TEXT("High penetration hit penetrates"), Penetrated.bPenetrated);
    TestFalse(TEXT("Penetrating hit is not stopped"), Penetrated.bStopped);
    TestTrue(TEXT("Penetrating hit keeps raw damage"), FMath::IsNearlyEqual(Penetrated.FinalDamage, 40.0f));
    TestTrue(TEXT("Penetrating hit still damages armor"), FMath::IsNearlyEqual(Penetrated.DurabilityAfter, 10.0f));

    Helmet->RegionalCoverageFraction = 0.50f;
    const float DurabilityBeforeMiss = Armor->GetArmorDurability(ETUEquipmentSlot::Headwear);
    const FTUArmorHitResult CoverageMiss = Armor->ResolveBallisticHit(
        ETUBodyRegion::Head,
        40.0f,
        10.0f,
        5.0f,
        0.75f);

    TestFalse(TEXT("Coverage miss does not resolve against armor"), CoverageMiss.bArmorPresent);
    TestTrue(TEXT("Coverage miss keeps raw damage"), FMath::IsNearlyEqual(CoverageMiss.FinalDamage, 40.0f));
    TestTrue(TEXT("Coverage miss does not reduce durability"),
        FMath::IsNearlyEqual(Armor->GetArmorDurability(ETUEquipmentSlot::Headwear), DurabilityBeforeMiss));

    UTUOperatorEquipmentComponent* EquipmentTwo = NewObject<UTUOperatorEquipmentComponent>();
    UTUArmorProtectionComponent* ArmorTwo = NewObject<UTUArmorProtectionComponent>();
    ArmorTwo->InitializeEquipment(EquipmentTwo);
    TestTrue(TEXT("Second operator equips same shared definition"), EquipmentTwo->EquipItem(Helmet));

    Helmet->RegionalCoverageFraction = 1.0f;
    const FTUArmorHitResult SecondOperatorHit = ArmorTwo->ResolveBallisticHit(
        ETUBodyRegion::Head,
        40.0f,
        10.0f,
        5.0f,
        0.5f);

    TestTrue(TEXT("Second operator starts from independent full durability"),
        FMath::IsNearlyEqual(SecondOperatorHit.DurabilityBefore, 20.0f));
    TestTrue(TEXT("First operator durability remains independent"),
        FMath::IsNearlyEqual(Armor->GetArmorDurability(ETUEquipmentSlot::Headwear), 10.0f));

    UTUEquipmentDefinition* ReplacementHelmet = NewObject<UTUEquipmentDefinition>();
    ReplacementHelmet->ItemId = TEXT("ballistic_helmet_replacement");
    ReplacementHelmet->Slot = ETUEquipmentSlot::Headwear;
    ReplacementHelmet->bProvidesBallisticProtection = true;
    ReplacementHelmet->ProtectedRegions.Add(ETUBodyRegion::Head);
    ReplacementHelmet->PenetrationResistance = 12.0f;
    ReplacementHelmet->MaxArmorDurability = 30.0f;
    ReplacementHelmet->StoppedRoundDamageMultiplier = 0.5f;
    ReplacementHelmet->RegionalCoverageFraction = 1.0f;

    TestTrue(TEXT("Replacing helmet succeeds"), Equipment->EquipItem(ReplacementHelmet));
    const FTUArmorHitResult ReplacementHit = Armor->ResolveBallisticHit(
        ETUBodyRegion::Head,
        20.0f,
        10.0f,
        2.0f,
        0.5f);

    TestTrue(TEXT("Replacement gets fresh independent durability"), FMath::IsNearlyEqual(ReplacementHit.DurabilityBefore, 30.0f));
    TestTrue(TEXT("Replacement uses its own residual multiplier"), FMath::IsNearlyEqual(ReplacementHit.FinalDamage, 10.0f));

    return true;
}

#endif
