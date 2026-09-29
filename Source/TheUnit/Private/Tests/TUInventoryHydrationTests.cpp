#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_WeaponBase.h"
#include "TU_AK105.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUInventoryHydrationTest, "TheUnit.Execution.Inventory.SavedClassAndAtomicImport",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTUInventoryHydrationTest::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!World || !GEngine) { if (World) World->DestroyWorld(false); AddError(TEXT("Test world unavailable")); return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ATU_ArmedOperatorCharacter* Source = World->SpawnActor<ATU_ArmedOperatorCharacter>();
    ATU_ArmedOperatorCharacter* Restored = World->SpawnActor<ATU_ArmedOperatorCharacter>();
    if (TestNotNull(TEXT("Source operator"), Source) && TestNotNull(TEXT("Restored operator"), Restored))
    {
        Source->SpawnDefaultWeapon();
        TestTrue(TEXT("Choose different authored primary"), Source->SelectPrimaryById(TEXT("PRIMARY_AK105")));
        const FTUItemLedger Saved = Source->ExportItemLedger();
        Restored->SpawnDefaultWeapon();
        TestTrue(TEXT("Saved kit imports"), Restored->ImportItemLedger(Saved));
        TestTrue(TEXT("Saved definition determines actual actor ballistics class"), Restored->GetPrimaryWeapon()->IsA<ATU_AK105>());
        TestEqual(TEXT("Primary item identity retained"), Restored->GetPrimaryWeapon()->GetWeaponInstanceId(), Source->GetPrimaryWeapon()->GetWeaponInstanceId());
        TestEqual(TEXT("Loaded ammo retained"), Restored->GetPrimaryWeapon()->GetCurrentAmmo(), Source->GetPrimaryWeapon()->GetCurrentAmmo());
        Restored->GetPrimaryWeapon()->StartReload();
        const FTUWeaponActionState ActionBefore = Restored->GetPrimaryWeapon()->GetActionState();
        FTUItemLedger Invalid = Restored->ExportItemLedger();
        if (TestTrue(TEXT("Two real weapon partitions exist"), Invalid.Weapons.Num() == 2))
        {
            Invalid.Weapons[1].ChamberAmmoId = TEXT("Unknown_Illegal_Ammo");
            TestFalse(TEXT("Invalid second partition rejects complete import"), Restored->ImportItemLedger(Invalid));
            const FTUWeaponActionState ActionAfter = Restored->GetPrimaryWeapon()->GetActionState();
            TestTrue(TEXT("Rejected import preserves active first action"), ActionBefore.bActive && ActionAfter.bActive);
            TestEqual(TEXT("Rejected import preserves action identity"), ActionAfter.ActionId, ActionBefore.ActionId);
            TestEqual(TEXT("Rejected import preserves revision"), ActionAfter.Revision, ActionBefore.Revision);
        }
        TestTrue(TEXT("Loss hydrates empty inventory"), Restored->ImportItemLedger(FTUItemLedger()));
        TestFalse(TEXT("Default spawning cannot mint lost kit"), Restored->SpawnDefaultWeapon());
        TestTrue(TEXT("Lost kit remains empty"), Restored->ExportItemLedger().Weapons.IsEmpty());
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
