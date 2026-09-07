#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_M110.h"
#include "TUWeaponAttachmentComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUM110RuntimeTest, "TheUnit.Combat.M110.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUM110RuntimeTest::RunTest(const FString& Parameters)
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

    ATU_M110* Weapon = World->SpawnActor<ATU_M110>();
    if (!TestNotNull(TEXT("M110 weapon actor"), Weapon))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("M110 weapon id"), Weapon->GetWeaponDefinition().WeaponId,
        FName(TEXT("WPN_M110_PrecisionDMR")));
    TestEqual(TEXT("M110 starts loaded"), Weapon->GetCurrentAmmo(), 20);
    TestEqual(TEXT("M110 reserve"), Weapon->GetReserveAmmo(), 80);
    TestEqual(TEXT("M110 ammo id"), Weapon->GetAmmoDefinition().AmmoId,
        FName(TEXT("Ammo_TU762_Precision")));
    TestEqual(TEXT("M110 default mode"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);

    Weapon->SetFireMode(ETUFireMode::FullAuto);
    TestEqual(TEXT("M110 remains semi-auto only"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);
    TestTrue(TEXT("M110 has positive cadence"), Weapon->GetFireIntervalSeconds() > 0.0f);

    UTUWeaponAttachmentComponent* Attachments = Weapon->GetAttachmentComponent();
    TestNotNull(TEXT("M110 uses shared modular attachment runtime"), Attachments);

    Weapon->SetAiming(true);
    TestTrue(TEXT("M110 uses shared ADS state"), Weapon->IsAiming());
    Weapon->FireSingleShot();
    TestEqual(TEXT("M110 shot consumes one round"), Weapon->GetCurrentAmmo(), 19);

    World->DestroyWorld(false);
    return true;
}

#endif
