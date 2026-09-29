#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_ModularOperatorCharacter.h"
#include "TU_WeaponBase.h"
#include "TU_GameMode.h"
#include "TU_PlayerState.h"
#include "TU_ExtractionZone.h"
#include "TUHealthComponent.h"

namespace TUExecutionIntegration
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GameInstance = nullptr;
        UTUHideoutLifecycleSubsystem* Lifecycle = nullptr;
        FString Slot = TEXT("TU_Automation_Integration_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
            World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
                true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                // World::Tick (including XR frame hooks) requires an engine context.
                // Keep this world on its own TimerManager; initializing GameInstance
                // subsystems here would load the ordinary user profile before isolation.
                GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            }
            GameInstance = NewObject<UGameInstance>();
            GameInstance->AddToRoot();
            Lifecycle = NewObject<UTUHideoutLifecycleSubsystem>(GameInstance);
            Lifecycle->AddToRoot();
            Lifecycle->ConfigureTestSlot(Slot);
            Lifecycle->LoadProfile();
        }

        ~FFixture()
        {
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
            Lifecycle->RemoveFromRoot();
            GameInstance->RemoveFromRoot();
            // Only this fixture's generated slots are removed, never live profile slots.
            for (const FString& Suffix : { FString(), FString(TEXT("_backup")), FString(TEXT("_journal")) })
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
        }

        UTUHideoutLifecycleSubsystem* Reload() const
        {
            UTUHideoutLifecycleSubsystem* Loaded = NewObject<UTUHideoutLifecycleSubsystem>(GameInstance);
            Loaded->ConfigureTestSlot(Slot);
            return Loaded->LoadProfile() ? Loaded : nullptr;
        }
    };

    int32 CountRounds(const FTUItemLedger& Ledger)
    {
        int32 Count = Ledger.LooseCartridges.Num();
        for (const FWeaponInstanceState& Weapon : Ledger.Weapons)
            Count += Weapon.ChamberAmmoId.IsNone() ? 0 : 1;
        for (const FTUMagazineInstance& Magazine : Ledger.Magazines)
            Count += Magazine.Cartridges.Num();
        return Count;
    }

    void SetOwner(FTUItemLedger& Ledger, const FGuid& Owner)
    {
        for (FWeaponInstanceState& Weapon : Ledger.Weapons) Weapon.OwnerId = Owner;
        for (FTUMagazineInstance& Magazine : Ledger.Magazines) Magazine.OwnerId = Owner;
        for (FTUItemInstance& Item : Ledger.Items) Item.OwnerId = Owner;
    }

    void AdvanceWorld(UWorld* World, float Seconds)
    {
        // TimerManager permits one tick per engine frame. Restore the global after
        // this synchronous isolated simulation so the editor's frame is unchanged.
        TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
        static uint64 SimulatedFrame = GFrameCounter;
        SimulatedFrame = FMath::Max(SimulatedFrame, GFrameCounter);
        const int32 Steps = FMath::CeilToInt(Seconds / 0.02f);
        for (int32 Step = 0; Step < Steps; ++Step)
        {
            GFrameCounter = ++SimulatedFrame;
            World->Tick(LEVELTICK_All, 0.02f);
        }
    }

    void ComparePayload(FAutomationTestBase& Test, const FTUItemLedger& Expected, const FTUItemLedger& Actual)
    {
        Test.TestEqual(TEXT("Round conservation across subsystem boundary"), CountRounds(Actual), CountRounds(Expected));
        Test.TestEqual(TEXT("Weapon count across subsystem boundary"), Actual.Weapons.Num(), Expected.Weapons.Num());
        Test.TestEqual(TEXT("Magazine count across subsystem boundary"), Actual.Magazines.Num(), Expected.Magazines.Num());
        Test.TestTrue(TEXT("Loose cartridge order retained"), Actual.LooseCartridges == Expected.LooseCartridges);
        for (const FWeaponInstanceState& Weapon : Expected.Weapons)
        {
            const FWeaponInstanceState* Found = Actual.Weapons.FindByPredicate([&](const FWeaponInstanceState& Value) { return Value.InstanceId == Weapon.InstanceId; });
            if (!Test.TestNotNull(TEXT("Durable weapon identity retained"), Found)) continue;
            Test.TestEqual(TEXT("Weapon owner retained"), Found->OwnerId, Weapon.OwnerId);
            Test.TestEqual(TEXT("Inserted magazine reference retained"), Found->InsertedMagazineId, Weapon.InsertedMagazineId);
            Test.TestEqual(TEXT("Chamber retained"), Found->ChamberAmmoId, Weapon.ChamberAmmoId);
            Test.TestEqual(TEXT("Action opening retained"), Found->bActionOpen, Weapon.bActionOpen);
            Test.TestEqual(TEXT("Condition retained"), Found->ConditionNormalized, Weapon.ConditionNormalized);
        }
        for (const FTUMagazineInstance& Magazine : Expected.Magazines)
        {
            const FTUMagazineInstance* Found = Actual.Magazines.FindByPredicate([&](const FTUMagazineInstance& Value) { return Value.InstanceId == Magazine.InstanceId; });
            if (!Test.TestNotNull(TEXT("Durable magazine identity retained"), Found)) continue;
            Test.TestEqual(TEXT("Magazine owner retained"), Found->OwnerId, Magazine.OwnerId);
            Test.TestEqual(TEXT("Magazine capacity retained"), Found->Capacity, Magazine.Capacity);
            Test.TestEqual(TEXT("Magazine containment retained"), Found->Location, Magazine.Location);
            Test.TestEqual(TEXT("Magazine weapon association retained"), Found->WeaponId, Magazine.WeaponId);
            Test.TestTrue(TEXT("Ordered magazine cartridges retained"), Found->Cartridges == Magazine.Cartridges);
        }
        for (const FTUWeaponActionState& Action : Expected.WeaponActions)
        {
            const FTUWeaponActionState* Found = Actual.WeaponActions.FindByPredicate([&](const auto& Value) { return Value.WeaponId == Action.WeaponId; });
            if (!Test.TestNotNull(TEXT("Durable action snapshot retained"), Found)) continue;
            Test.TestEqual(TEXT("Committed action phase retained"), Found->Phase, Action.Phase);
            Test.TestEqual(TEXT("Outgoing magazine identity retained"), Found->OutgoingMagazineId, Action.OutgoingMagazineId);
            Test.TestEqual(TEXT("Replacement magazine identity retained"), Found->ReplacementMagazineId, Action.ReplacementMagazineId);
            Test.TestEqual(TEXT("Interrupted activity retained"), Found->bActive, Action.bActive);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUExecutionActorPersistenceTest,
    "TheUnit.Execution.Integration.ActorEscrowSaveReload",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTUExecutionActorPersistenceTest::RunTest(const FString& Parameters)
{
    using namespace TUExecutionIntegration;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Isolated world"), Fixture.World) || !TestNotNull(TEXT("Isolated profile"), Fixture.Lifecycle->GetProfile())) return false;
    ATU_WeaponBase* Weapon = Fixture.World->SpawnActor<ATU_WeaponBase>();
    if (!TestNotNull(TEXT("Canonical runtime weapon"), Weapon)) return false;
    FTUItemLedger Kit = Weapon->ExportItemLedger();
    SetOwner(Kit, Fixture.Lifecycle->GetLocalPlayerId());
    if (!TestTrue(TEXT("Hydrate owned weapon ledger"), Weapon->ImportItemLedger(Kit))) return false;
    if (!TestTrue(TEXT("Persist initial kit once"), Fixture.Lifecycle->CaptureInitialKit(Kit))) return false;
    const FGuid RaidId = FGuid::NewGuid();
    if (!TestTrue(TEXT("Escrow before raid"), Fixture.Lifecycle->BeginDeployment(RaidId, Fixture.Lifecycle->GetLocalPlayerId(), Kit, false))) return false;
    TestTrue(TEXT("Deployed unique weapons unavailable in stash"), Fixture.Lifecycle->GetProfile()->Stash.Weapons.IsEmpty());
    Weapon->Fire();
    const FTUItemLedger Spent = Weapon->ExportItemLedger();
    TestEqual(TEXT("Actor shot spends one durable cartridge"), CountRounds(Spent), CountRounds(Kit) - 1);

    FTURaidOutcome Outcome;
    Outcome.RaidId = RaidId;
    Outcome.PlayerId = Fixture.Lifecycle->GetLocalPlayerId();
    Outcome.Outcome = ETURaidPlayerOutcome::Extracted;
    Outcome.ExtractId = TEXT("QA_North");
    FTUTaskProgress Unfinished;
    Unfinished.PlayerId = Outcome.PlayerId;
    Unfinished.Definition.TaskId = TEXT("QA_Unfinished");
    Unfinished.Definition.RequiredCount = 3;
    Unfinished.CommittedCount = 1;
    const TArray<FTUTaskProgress> Tasks { Unfinished };
    Fixture.Lifecycle->InjectSaveFailure(3);
    TestFalse(TEXT("Primary write failure does not acknowledge extracted kit"), Fixture.Lifecycle->CommitRaidOutcome(Outcome, Spent, Tasks));
    TestTrue(TEXT("Failure leaves live stash unavailable"), Fixture.Lifecycle->GetProfile()->Stash.Weapons.IsEmpty());
    Fixture.Lifecycle->InjectSaveFailure(0);
    if (!TestTrue(TEXT("Recoverable outcome retry"), Fixture.Lifecycle->CommitRaidOutcome(Outcome, Spent, Tasks))) return false;
    const int32 Completed = Fixture.Lifecycle->GetProfile()->CompletedOperations;
    Fixture.Lifecycle->CommitRaidOutcome(Outcome, Spent, Tasks);
    TestEqual(TEXT("Repeated result does not reaward progression"), Fixture.Lifecycle->GetProfile()->CompletedOperations, Completed);

    UTUHideoutLifecycleSubsystem* Reloaded = Fixture.Reload();
    if (!TestNotNull(TEXT("Fresh subsystem loads isolated disk profile"), Reloaded)) return false;
    ComparePayload(*this, Spent, Reloaded->GetProfile()->Stash);
    TestEqual(TEXT("One terminal outcome survived disk reload"), Reloaded->GetProfile()->Outcomes.Num(), 1);
    TestEqual(TEXT("One unfinished task survived disk reload"), Reloaded->GetProfile()->Tasks.Num(), 1);
    if (!Reloaded->GetProfile()->Tasks.IsEmpty())
        TestFalse(TEXT("Ordinary extraction did not complete unfinished task"), Reloaded->GetProfile()->Tasks[0].bCompleted);
    ATU_WeaponBase* Restored = Fixture.World->SpawnActor<ATU_WeaponBase>();
    if (!TestNotNull(TEXT("Replacement runtime representation"), Restored)) return false;
    TestTrue(TEXT("Saved ledger hydrates canonical actor"), Restored->ImportItemLedger(Reloaded->GetProfile()->Stash));
    ComparePayload(*this, Spent, Restored->ExportItemLedger());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUExecutionInterruptedKitPersistenceTest,
    "TheUnit.Execution.Integration.SwitchInterruptSaveReload",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTUExecutionInterruptedKitPersistenceTest::RunTest(const FString& Parameters)
{
    using namespace TUExecutionIntegration;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Isolated world"), Fixture.World)) return false;
    ATU_ArmedOperatorCharacter* Operator = Fixture.World->SpawnActor<ATU_ArmedOperatorCharacter>();
    if (!TestNotNull(TEXT("Actual armed player character"), Operator)
        || !TestTrue(TEXT("Spawn real carried slots"), Operator->SpawnDefaultWeapon())) return false;
    FTUItemLedger Kit = Operator->ExportItemLedger();
    SetOwner(Kit, Fixture.Lifecycle->GetLocalPlayerId());
    if (!TestTrue(TEXT("Restore complete owned kit"), Operator->ImportItemLedger(Kit))) return false;
    if (!TestTrue(TEXT("Capture complete kit"), Fixture.Lifecycle->CaptureInitialKit(Kit))) return false;
    const FGuid RaidId = FGuid::NewGuid();
    if (!TestTrue(TEXT("Escrow complete kit"), Fixture.Lifecycle->BeginDeployment(RaidId, Fixture.Lifecycle->GetLocalPlayerId(), Kit, false))) return false;
    ATU_WeaponBase* Primary = Operator->GetPrimaryWeapon();
    if (!TestNotNull(TEXT("Primary representation"), Primary)) return false;
    Primary->Fire();
    if (!TestTrue(TEXT("Reload request uses canonical authority path"), Primary->RequestReload(
        FGuid::NewGuid(), Primary->ExportItemLedger().Revision, ETUReloadPolicy::Retain))) return false;
    // Stop as soon as the real authority scheduler commits removal.
    for (int32 Step = 0; Step < 250 && Primary->GetActionState().Phase == ETUWeaponActionPhase::Begin; ++Step)
        AdvanceWorld(Fixture.World, 0.02f);
    TestEqual(TEXT("Authority schedule reached removal"), Primary->GetActionState().Phase, ETUWeaponActionPhase::Removed);
    if (!TestTrue(TEXT("Switch through actual player API"), Operator->EquipWeaponSlot(ETUOperatorWeaponSlot::Secondary))) return false;
    TestFalse(TEXT("Switch interrupts action"), Primary->GetActionState().bActive);
    const FTUItemLedger Interrupted = Operator->ExportItemLedger();
    AdvanceWorld(Fixture.World, 5.f);
    ComparePayload(*this, Interrupted, Operator->ExportItemLedger());
    TestFalse(TEXT("Hidden weapon cannot finish abandoned timer"), Primary->GetActionState().bActive);

    FTURaidOutcome Outcome;
    Outcome.RaidId = RaidId;
    Outcome.PlayerId = Fixture.Lifecycle->GetLocalPlayerId();
    Outcome.Outcome = ETURaidPlayerOutcome::Extracted;
    if (!TestTrue(TEXT("Persist interrupted physical state"), Fixture.Lifecycle->CommitRaidOutcome(Outcome, Interrupted, {}))) return false;
    UTUHideoutLifecycleSubsystem* Reloaded = Fixture.Reload();
    if (!TestNotNull(TEXT("Reload interrupted state from disk"), Reloaded)) return false;
    ATU_ArmedOperatorCharacter* Restored = Fixture.World->SpawnActor<ATU_ArmedOperatorCharacter>();
    if (!TestNotNull(TEXT("Fresh player character representation"), Restored)) return false;
    TestTrue(TEXT("Hydrate all saved carried weapon slots"), Restored->ImportItemLedger(Reloaded->GetProfile()->Stash));
    ComparePayload(*this, Interrupted, Restored->ExportItemLedger());
    TestTrue(TEXT("Re-equip restored primary"), Restored->EquipWeaponSlot(ETUOperatorWeaponSlot::Primary));
    ComparePayload(*this, Interrupted, Restored->ExportItemLedger());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUExecutionSplitRaidPersistenceTest,
    "TheUnit.Execution.Integration.SplitRaidDurableOutcomes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTUExecutionSplitRaidPersistenceTest::RunTest(const FString& Parameters)
{
    using namespace TUExecutionIntegration;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Isolated world"), Fixture.World)) return false;
    ATU_GameMode* Mode = Fixture.World->SpawnActor<ATU_GameMode>();
    if (!TestNotNull(TEXT("Real raid authority"), Mode)) return false;
    Mode->ConfigureTestLifecycle(Fixture.Lifecycle);
    Mode->StartRaid(FGuid::NewGuid(), 60.f, false);
    ATU_ArmedOperatorCharacter* Players[2] = {};
    FGuid PlayerIds[2];
    for (int32 Index = 0; Index < 2; ++Index)
    {
        Players[Index] = Fixture.World->SpawnActor<ATU_ModularOperatorCharacter>();
        if (!TestNotNull(TEXT("Distinct real player pawn"), Players[Index])) return false;
        Players[Index]->SetActorLocation(FVector(Index * 1000.f, 0.f, 0.f));
        Players[Index]->SpawnDefaultWeapon();
        ATU_PlayerState* State = Fixture.World->SpawnActor<ATU_PlayerState>();
        if (!TestNotNull(TEXT("Distinct authoritative player state"), State)) return false;
        State->PersistentPlayerId = FGuid::NewGuid();
        Players[Index]->SetPlayerState(State);
        FTUItemLedger Kit = Players[Index]->ExportItemLedger();
        SetOwner(Kit, State->PersistentPlayerId);
        if (!TestTrue(TEXT("Hydrate separately owned kit"), Players[Index]->ImportItemLedger(Kit))) return false;
        PlayerIds[Index] = Mode->RegisterParticipant(Players[Index]);
        if (!TestTrue(TEXT("Participant deployment persisted"), PlayerIds[Index].IsValid())) return false;
    }
    TestTrue(TEXT("Split players have distinct persistent identities"), PlayerIds[0] != PlayerIds[1]);
    FTUTaskDefinition Task;
    Task.TaskId = TEXT("QA_Split_Unfinished");
    Task.TargetId = TEXT("QA_Split_Object");
    Task.RequiredCount = 2;
    TestTrue(TEXT("Add unfinished individual task"), Mode->AddTask(Players[0], Task));
    TestTrue(TEXT("Actual authority credits one interaction"), Mode->RecordUniqueTaskEvent(Players[0], FGuid::NewGuid(), ETUTaskCondition::Interact, Task.TargetId));
    const FTUItemLedger ExtractedKit = Players[0]->ExportItemLedger();
    ATU_ExtractionZone* Exit = Fixture.World->SpawnActor<ATU_ExtractionZone>();
    if (!TestNotNull(TEXT("Real extraction zone"), Exit)) return false;
    Exit->SetActorLocation(Players[0]->GetActorLocation());
    Exit->HoldSeconds = 1.f;
    TestTrue(TEXT("Unfinished task does not block exit countdown"), Mode->BeginExtraction(Players[0], Exit));
    Mode->AdvanceRaidTime(1.1f);
    const FTURaidParticipantState* A = Mode->FindParticipant(Players[0]);
    const FTURaidParticipantState* B = Mode->FindParticipant(Players[1]);
    if (!TestNotNull(TEXT("Extracted participant remains recorded"), A) || !TestNotNull(TEXT("Remaining participant remains recorded"), B)) return false;
    TestEqual(TEXT("First participant extracted"), A->Outcome, ETURaidPlayerOutcome::Extracted);
    TestTrue(TEXT("Extraction durable before acknowledged"), A->bOutcomePersisted);
    TestEqual(TEXT("Teammate remains active in same raid"), B->Outcome, ETURaidPlayerOutcome::Active);
    TestFalse(TEXT("Teammate remains visible"), Players[1]->IsHidden());
    TestFalse(TEXT("Task remains unfinished"), A->Tasks.IsEmpty() || A->Tasks[0].bCompleted);
    UTUHealthComponent* Health = Players[1]->FindComponentByClass<UTUHealthComponent>();
    if (!TestNotNull(TEXT("Real health component"), Health)) return false;
    Health->ApplyRegionalDamage(ETUBodyRegion::Head, 100000.f);
    Mode->AdvanceRaidTime(0.1f);
    TestEqual(TEXT("Second participant death through health and raid tick"), Mode->FindParticipant(Players[1])->Outcome, ETURaidPlayerOutcome::Dead);
    TestFalse(TEXT("Dead participant cannot replay an extraction"), Mode->ResolvePlayerOutcome(Players[1], ETURaidPlayerOutcome::Extracted, Exit->ExtractId));
    UTUHideoutLifecycleSubsystem* Reloaded = Fixture.Reload();
    if (!TestNotNull(TEXT("Reload durable split results"), Reloaded)) return false;
    TestEqual(TEXT("Two independent terminal records"), Reloaded->GetProfile()->Outcomes.Num(), 2);
    ComparePayload(*this, ExtractedKit, Reloaded->GetPlayerStash(PlayerIds[0]));
    TestTrue(TEXT("Dead participant's weapons lost"), Reloaded->GetPlayerStash(PlayerIds[1]).Weapons.IsEmpty());
    TestTrue(TEXT("Host local stash never receives guest kit"), Reloaded->GetPlayerStash(Reloaded->GetLocalPlayerId()).Weapons.IsEmpty());
    return true;
}

#endif
