#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_AK105.h"
#include "TUWeaponAttachmentComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUAK105RuntimeTest, "TheUnit.Combat.AK105.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUAK105RuntimeTest::RunTest(const FString& Parameters)
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

    ATU_AK105* Weapon = World->SpawnActor<ATU_AK105>();
    if (!TestNotNull(TEXT("AK-105 weapon actor"), Weapon))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("AK-105 weapon id"), Weapon->GetWeaponDefinition().WeaponId, FName(TEXT("WPN_AK105_Modernized")));
    TestEqual(TEXT("AK-105 starts loaded"), Weapon->GetCurrentAmmo(), 30);
    TestEqual(TEXT("AK-105 reserve"), Weapon->GetReserveAmmo(), 120);
    TestEqual(TEXT("AK-105 ammo id"), Weapon->GetAmmoDefinition().AmmoId, FName(TEXT("Ammo_TU545_Ball")));
    TestEqual(TEXT("AK-105 default mode"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);

    Weapon->SetFireMode(ETUFireMode::FullAuto);
    TestEqual(TEXT("AK-105 supports full auto"), Weapon->GetCurrentFireMode(), ETUFireMode::FullAuto);
    TestTrue(TEXT("AK-105 has positive cadence"), Weapon->GetFireIntervalSeconds() > 0.0f);

    UTUWeaponAttachmentComponent* Attachments = Weapon->GetAttachmentComponent();
    TestNotNull(TEXT("AK-105 uses shared modular attachment runtime"), Attachments);

    World->DestroyWorld(false);
    return true;
}

#endif
