#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_RGRFive7.h"
#include "TUWeaponAttachmentComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTURGRFive7RuntimeTest, "TheUnit.Combat.RGRFive7.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTURGRFive7RuntimeTest::RunTest(const FString& Parameters)
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

    ATU_RGRFive7* Weapon = World->SpawnActor<ATU_RGRFive7>();
    if (!TestNotNull(TEXT("RGR Five7 weapon actor"), Weapon))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("RGR Five7 weapon id"), Weapon->GetWeaponDefinition().WeaponId,
        FName(TEXT("WPN_RGRFive7_TacticalPistol")));
    TestEqual(TEXT("RGR Five7 starts loaded"), Weapon->GetCurrentAmmo(), 20);
    TestEqual(TEXT("RGR Five7 reserve"), Weapon->GetReserveAmmo(), 100);
    TestEqual(TEXT("RGR Five7 ammo id"), Weapon->GetAmmoDefinition().AmmoId,
        FName(TEXT("Ammo_TU57_Ball")));
    TestEqual(TEXT("RGR Five7 default mode"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);

    Weapon->SetFireMode(ETUFireMode::FullAuto);
    TestEqual(TEXT("RGR Five7 remains semi-auto only"), Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);
    TestTrue(TEXT("RGR Five7 has positive cadence"), Weapon->GetFireIntervalSeconds() > 0.0f);

    UTUWeaponAttachmentComponent* Attachments = Weapon->GetAttachmentComponent();
    TestNotNull(TEXT("RGR Five7 uses shared modular attachment runtime"), Attachments);

    Weapon->SetAiming(true);
    TestTrue(TEXT("RGR Five7 uses shared ADS state"), Weapon->IsAiming());
    Weapon->FireSingleShot();
    TestEqual(TEXT("RGR Five7 shot consumes one round"), Weapon->GetCurrentAmmo(), 19);

    World->DestroyWorld(false);
    return true;
}

#endif
