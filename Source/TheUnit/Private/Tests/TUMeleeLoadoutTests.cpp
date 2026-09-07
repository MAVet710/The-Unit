#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_Karambit.h"
#include "TU_OTFKnife.h"
#include "TUMeleeLoadoutComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUMeleeLoadoutComponentTest, "TheUnit.Combat.Melee.Loadout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUMeleeLoadoutComponentTest::RunTest(const FString& Parameters)
{
    UTUMeleeLoadoutComponent* Loadout = NewObject<UTUMeleeLoadoutComponent>();
    if (!TestNotNull(TEXT("Melee loadout component"), Loadout))
    {
        return false;
    }

    TestEqual(TEXT("Two built-in melee choices"), Loadout->GetAvailableItems().Num(), 2);
    TestEqual(TEXT("OTF selected by default"), Loadout->GetSelectedItemId(), FName(TEXT("MELEE_OTF")));
    TestTrue(TEXT("Default class is OTF"), Loadout->GetSelectedMeleeClass() == ATU_OTFKnife::StaticClass());
    TestTrue(TEXT("Default melee has non-negative inventory weight"), Loadout->GetSelectedWeightKg() >= 0.0f);

    TestTrue(TEXT("Karambit can be selected by id"), Loadout->SelectItemById(TEXT("MELEE_Karambit")));
    TestTrue(TEXT("Selected class becomes karambit"), Loadout->GetSelectedMeleeClass() == ATU_Karambit::StaticClass());

    TestTrue(TEXT("Forward cycle wraps to OTF"), Loadout->CycleSelection(1));
    TestEqual(TEXT("Forward cycle selected OTF"), Loadout->GetSelectedItemId(), FName(TEXT("MELEE_OTF")));
    TestTrue(TEXT("Backward cycle wraps to karambit"), Loadout->CycleSelection(-1));
    TestEqual(TEXT("Backward cycle selected karambit"), Loadout->GetSelectedItemId(), FName(TEXT("MELEE_Karambit")));

    FTUMeleeEquipmentEntry Custom;
    Custom.ItemId = TEXT("MELEE_Test");
    Custom.DisplayName = FText::FromString(TEXT("Test Melee"));
    Custom.MeleeClass = ATU_Karambit::StaticClass();
    Custom.EquipSocket = TEXT("test_melee_socket");
    Custom.WeightKg = 0.42f;

    TestTrue(TEXT("Custom item can be added"), Loadout->AddOrReplaceItem(Custom));
    TestTrue(TEXT("Custom item can be selected"), Loadout->SelectItemById(Custom.ItemId));
    TestEqual(TEXT("Custom selected weight"), Loadout->GetSelectedWeightKg(), 0.42f);

    Custom.WeightKg = 0.55f;
    TestTrue(TEXT("Duplicate id replaces instead of duplicating"), Loadout->AddOrReplaceItem(Custom));
    TestEqual(TEXT("Replacement does not change item count"), Loadout->GetAvailableItems().Num(), 3);
    TestEqual(TEXT("Replacement updates selected weight"), Loadout->GetSelectedWeightKg(), 0.55f);

    TArray<FTUMeleeEquipmentEntry> OneItemLoadout;
    OneItemLoadout.Add(Custom);
    Loadout->SetItems(OneItemLoadout, Custom.ItemId);
    TestEqual(TEXT("SetItems replaces inventory"), Loadout->GetAvailableItems().Num(), 1);
    TestEqual(TEXT("Preferred item remains selected"), Loadout->GetSelectedItemId(), Custom.ItemId);

    TestFalse(TEXT("Unknown item cannot be selected"), Loadout->SelectItemById(TEXT("MELEE_Missing")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUOperatorMeleeSelectionTest, "TheUnit.Combat.Melee.OperatorSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUOperatorMeleeSelectionTest::RunTest(const FString& Parameters)
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

    TestNotNull(TEXT("Operator owns melee loadout"), Operator->GetMeleeLoadout());
    TestEqual(TEXT("Operator starts with OTF selected"), Operator->GetSelectedMeleeId(), FName(TEXT("MELEE_OTF")));
    TestTrue(TEXT("Operator can spawn selected melee"), Operator->SpawnDefaultMelee());
    TestTrue(TEXT("Spawned default melee is exactly OTF"), Operator->GetCurrentMelee() && Operator->GetCurrentMelee()->GetClass() == ATU_OTFKnife::StaticClass());

    TestTrue(TEXT("Operator can select karambit while holstered"), Operator->SelectMeleeById(TEXT("MELEE_Karambit")));
    TestEqual(TEXT("Karambit id becomes selected"), Operator->GetSelectedMeleeId(), FName(TEXT("MELEE_Karambit")));
    TestTrue(TEXT("Runtime melee actor is replaced with karambit"), Operator->GetCurrentMelee() && Operator->GetCurrentMelee()->IsA<ATU_Karambit>());

    TestTrue(TEXT("Operator can cycle back to OTF"), Operator->CycleMeleeSelection(1));
    TestEqual(TEXT("Cycle returns selected id to OTF"), Operator->GetSelectedMeleeId(), FName(TEXT("MELEE_OTF")));
    TestTrue(TEXT("Runtime melee actor returns to OTF class"), Operator->GetCurrentMelee() && Operator->GetCurrentMelee()->GetClass() == ATU_OTFKnife::StaticClass());

    TestTrue(TEXT("Selected melee can be drawn"), Operator->DrawMelee());
    TestTrue(TEXT("Operator reports melee equipped"), Operator->IsMeleeEquipped());
    TestFalse(TEXT("Selection changes are blocked while melee is drawn"), Operator->SelectMeleeById(TEXT("MELEE_Karambit")));
    TestFalse(TEXT("Cycling is blocked while melee is drawn"), Operator->CycleMeleeSelection(1));

    World->DestroyWorld(false);
    return true;
}

#endif
