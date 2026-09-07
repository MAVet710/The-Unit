#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_G34CM.h"
#include "TUWeaponAttachmentComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUG34CMRuntimeTest, "TheUnit.Combat.G34CM.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUG34CMRuntimeTest::RunTest(const FString& Parameters)
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

    ATU_G34CM* Weapon = World->SpawnActor<ATU_G34CM>();
    if (!TestNotNull(TEXT("G34 CM weapon actor"), Weapon))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("G34 CM weapon id"), Weapon->GetWeaponDefinition().WeaponId,
        FName(TEXT("WPN_G34CM_CompetitionPistol")));
    TestEqual(TEXT("G34 CM starts loaded"), Weapon->GetCurrentAmmo(), 20);
    TestEqual(TEXT("G34 CM reserve"), Weapon->GetReserveAmmo(), 100);
    TestEqual(TEXT("G34 CM ammo id"), Weapon->GetAmmoDefinition().AmmoId,
        FName(TEXT("Ammo_TU9_Ball")));
    TestEqual(TEXT("G34 CM default mode"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);

    Weapon->SetFireMode(ETUFireMode::FullAuto);
    TestEqual(TEXT("G34 CM remains semi-auto only"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);
    TestTrue(TEXT("G34 CM has positive cadence"), Weapon->GetFireIntervalSeconds() > 0.0f);

    UTUWeaponAttachmentComponent* Attachments = Weapon->GetAttachmentComponent();
    TestNotNull(TEXT("G34 CM uses shared modular attachment runtime"), Attachments);

    Weapon->SetAiming(true);
    TestTrue(TEXT("G34 CM uses shared ADS state"), Weapon->IsAiming());
    Weapon->FireSingleShot();
    TestEqual(TEXT("G34 CM shot consumes one round"), Weapon->GetCurrentAmmo(), 19);

    World->DestroyWorld(false);
    return true;
}

#endif
