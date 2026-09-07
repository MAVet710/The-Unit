#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_TacticalRifle.h"
#include "TUWeaponAttachmentComponent.h"
#include "TUWeaponAttachmentDefinition.h"
#include "TUWeaponLoadoutData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUTacticalRifleRuntimeTest, "TheUnit.Combat.TacticalRifle.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUTacticalRifleRuntimeTest::RunTest(const FString& Parameters)
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

    ATU_TacticalRifle* Rifle = World->SpawnActor<ATU_TacticalRifle>();
    if (!TestNotNull(TEXT("Tactical rifle"), Rifle))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("Rifle id"), Rifle->GetWeaponDefinition().WeaponId, FName(TEXT("WPN_TU556_ModularCarbine")));
    TestEqual(TEXT("Initial loaded rounds"), Rifle->GetCurrentAmmo(), 30);
    TestEqual(TEXT("Initial reserve"), Rifle->GetReserveAmmo(), 120);
    TestTrue(TEXT("Timed fire interval derives from authored RPM"),
        FMath::IsNearlyEqual(Rifle->GetFireIntervalSeconds(), 60.0f / 700.0f, KINDA_SMALL_NUMBER));
    TestNotNull(TEXT("Rifle owns visual body component"), Rifle->GetWeaponBodyMesh());
    TestNotNull(TEXT("Rifle owns attachment component"), Rifle->GetAttachmentComponent());

    Rifle->StartFire();
    TestEqual(TEXT("First semi-auto trigger press fires one shot"), Rifle->GetCurrentAmmo(), 29);
    Rifle->StartFire();
    TestEqual(TEXT("Cadence gate blocks same-frame second shot"), Rifle->GetCurrentAmmo(), 29);

    Rifle->StartReload();
    TestTrue(TEXT("Runtime reload enters timed reload state"), Rifle->IsReloading());
    TestEqual(TEXT("Timed reload does not instantly mutate ammo"), Rifle->GetCurrentAmmo(), 29);
    Rifle->FinishReload();
    TestFalse(TEXT("Manual completion exits reload state"), Rifle->IsReloading());
    TestEqual(TEXT("Tactical reload restores capacity plus chamber"), Rifle->GetCurrentAmmo(), 31);

    UTUWeaponAttachmentComponent* Attachments = NewObject<UTUWeaponAttachmentComponent>();
    UTUWeaponAttachmentDefinition* Optic = NewObject<UTUWeaponAttachmentDefinition>();
    Optic->ItemId = TEXT("optic_reference");
    Optic->Slot = ETUWeaponAttachmentSlot::Optic;
    Optic->SpreadMultiplier = 0.90f;
    Optic->WeightKg = 0.30f;

    UTUWeaponAttachmentDefinition* Foregrip = NewObject<UTUWeaponAttachmentDefinition>();
    Foregrip->ItemId = TEXT("foregrip_reference");
    Foregrip->Slot = ETUWeaponAttachmentSlot::Foregrip;
    Foregrip->RecoilMultiplier = 0.85f;
    Foregrip->WeightKg = 0.12f;

    TestTrue(TEXT("Equip optic"), Attachments->EquipAttachment(Optic));
    TestTrue(TEXT("Equip foregrip"), Attachments->EquipAttachment(Foregrip));
    TestEqual(TEXT("Optic occupies optic slot"), Attachments->GetAttachment(ETUWeaponAttachmentSlot::Optic), Optic);
    TestTrue(TEXT("Attachment spread modifier aggregates"), FMath::IsNearlyEqual(Attachments->GetSpreadMultiplier(), 0.90f));
    TestTrue(TEXT("Attachment recoil modifier aggregates"), FMath::IsNearlyEqual(Attachments->GetRecoilMultiplier(), 0.85f));
    TestTrue(TEXT("Attachment weight sums"), FMath::IsNearlyEqual(Attachments->GetAttachmentWeightKg(), 0.42f));

    UTUWeaponAttachmentDefinition* ReplacementOptic = NewObject<UTUWeaponAttachmentDefinition>();
    ReplacementOptic->ItemId = TEXT("optic_replacement");
    ReplacementOptic->Slot = ETUWeaponAttachmentSlot::Optic;
    TestTrue(TEXT("Same-slot replacement succeeds"), Attachments->EquipAttachment(ReplacementOptic));
    TestEqual(TEXT("Replacement owns optic slot"), Attachments->GetAttachment(ETUWeaponAttachmentSlot::Optic), ReplacementOptic);

    ATU_ArmedOperatorCharacter* ArmedOperator = World->SpawnActor<ATU_ArmedOperatorCharacter>();
    if (TestNotNull(TEXT("Armed operator"), ArmedOperator))
    {
        TestTrue(TEXT("Armed operator can spawn its default weapon"), ArmedOperator->SpawnDefaultWeapon());
        ATU_WeaponBase* EquippedWeapon = ArmedOperator->GetCurrentWeapon();
        if (TestNotNull(TEXT("Armed operator owns weapon"), EquippedWeapon))
        {
            TestTrue(TEXT("Default weapon is tactical rifle"), EquippedWeapon->IsA<ATU_TacticalRifle>());
        }
    }

    World->DestroyWorld(false);
    return true;
}

#endif
