#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "TUPersistence.h"

namespace
{
struct FPersistenceFixture
{
    FString Slot = TEXT("TU_Automation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    UGameInstance* Game = NewObject<UGameInstance>();
    UTUHideoutLifecycleSubsystem* Store = nullptr;
    FPersistenceFixture() { Game->AddToRoot(); Store = NewObject<UTUHideoutLifecycleSubsystem>(Game); Store->ConfigureTestSlot(Slot); Store->LoadProfile(); }
    ~FPersistenceFixture() { for (const FString& Suffix : { FString(), FString(TEXT("_backup")), FString(TEXT("_journal")) }) UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0); Game->RemoveFromRoot(); }
    UTUHideoutLifecycleSubsystem* Reload() { auto* S = NewObject<UTUHideoutLifecycleSubsystem>(Game); S->ConfigureTestSlot(Slot); S->LoadProfile(); return S; }
};
FTUItemLedger Kit(FGuid Owner)
{
    FTUItemLedger L;
    FWeaponInstanceState W; W.InstanceId = FGuid::NewGuid(); W.OwnerId = Owner; W.DefinitionId = TEXT("WPN_TU556"); W.LoadoutSlot = TEXT("Primary"); W.ChamberAmmoId = TEXT("556"); W.ConditionNormalized = 0.75f;
    FTUMagazineInstance M; M.InstanceId = FGuid::NewGuid(); M.OwnerId = Owner; M.WeaponId = W.InstanceId; M.CompatibleAmmoId = TEXT("556"); M.Location = ETUItemLocation::Inserted; M.Cartridges.Init(TEXT("556"), 30); W.InsertedMagazineId = M.InstanceId;
    L.Weapons.Add(W); L.Magazines.Add(M);
    M.InstanceId = FGuid::NewGuid(); M.WeaponId.Invalidate(); M.Location = ETUItemLocation::Carried; L.Magazines.Add(M);
    return L;
}
FTURaidOutcome Result(FGuid Raid, FGuid Player, ETURaidPlayerOutcome Outcome = ETURaidPlayerOutcome::Extracted)
{
    FTURaidOutcome R; R.RaidId = Raid; R.PlayerId = Player; R.Outcome = Outcome; R.ExtractId = TEXT("North"); return R;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPersistenceFailureTest, "TheUnit.Persistence.DurableFailureAndReplay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPersistenceFailureTest::RunTest(const FString&)
{
    for (int32 Failure = 1; Failure <= 3; ++Failure)
    {
        FPersistenceFixture F; auto* S = F.Store; const FGuid Player = S->GetLocalPlayerId(), Raid = FGuid::NewGuid(); const auto L = Kit(Player);
        TestTrue(TEXT("Seed authored kit"), S->CaptureInitialKit(L));
        TestTrue(TEXT("Deploy durable escrow"), S->BeginDeployment(Raid, Player, L, false));
        TestEqual(TEXT("Deployed weapons removed from stash"), S->GetProfile()->Stash.Weapons.Num(), 0);
        FTUItemLedger Returned = L; Returned.Magazines[0].Cartridges.RemoveAt(0,11);
        FTUTaskProgress Task; Task.PlayerId = Player; Task.Definition.TaskId = TEXT("Visit"); Task.CommittedCount = 1;
        S->InjectSaveFailure(Failure);
        TestFalse(TEXT("Injected write failure denies committed extraction"), S->CommitRaidOutcome(Result(Raid,Player), Returned, {Task}));
        TestEqual(TEXT("Failure cannot award"), S->GetProfile()->CompletedOperations, 0);
        auto* Restart = F.Reload();
        TestEqual(TEXT("Relaunch retains unresolved deployment"), Restart->GetActiveRaidId(), Raid);
        TestEqual(TEXT("Relaunch has no award from prepared journal"), Restart->GetProfile()->CompletedOperations, 0);
        TestTrue(TEXT("Retry commits"), Restart->CommitRaidOutcome(Result(Raid,Player), Returned, {Task}));
        TestTrue(TEXT("Exact duplicate acknowledged"), Restart->CommitRaidOutcome(Result(Raid,Player), Returned, {Task}));
        TestFalse(TEXT("Conflicting terminal rejected"), Restart->CommitRaidOutcome(Result(Raid,Player,ETURaidPlayerOutcome::Dead), Returned, {}));
        auto* Saved = F.Reload();
        TestEqual(TEXT("One completion after relaunch"), Saved->GetProfile()->CompletedOperations, 1);
        TestEqual(TEXT("Stable weapon ID"), Saved->GetProfile()->Stash.Weapons[0].InstanceId, L.Weapons[0].InstanceId);
        TestEqual(TEXT("Exact fired magazine remainder"), Saved->GetProfile()->Stash.Magazines[0].Cartridges.Num(), 19);
        TestEqual(TEXT("Condition survives serialization"), Saved->GetProfile()->Stash.Weapons[0].ConditionNormalized, 0.75f);
        TestEqual(TEXT("Task committed exactly once"), Saved->GetProfile()->Tasks.Num(), 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPersistenceOwnershipTest, "TheUnit.Persistence.OwnershipTrainingAndSplit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPersistenceOwnershipTest::RunTest(const FString&)
{
    FPersistenceFixture F; auto* S=F.Store; FGuid P=S->GetLocalPlayerId(), Other=FGuid::NewGuid(), Raid=FGuid::NewGuid(); auto L=Kit(P), R=Kit(Other);
    TestTrue(TEXT("Seed local"),S->CaptureInitialKit(L)); TestTrue(TEXT("Seed server-known teammate"),S->CaptureInitialKitForPlayer(Other,R));
    auto Forged=L; Forged.Weapons[0].OwnerId=Other;
    TestFalse(TEXT("Cross-player ownership invalid"),S->BeginDeployment(Raid,P,Forged,false));
    TestTrue(TEXT("Local deployment"),S->BeginDeployment(Raid,P,L,false)); TestTrue(TEXT("Teammate deployment"),S->BeginDeployment(Raid,Other,R,false));
    Forged=L; Forged.Magazines[0].Cartridges.Add(TEXT("556"));
    TestFalse(TEXT("Ammo mint invalid"),S->CommitRaidOutcome(Result(Raid,P),Forged,{}));
    TestTrue(TEXT("One extracts"),S->CommitRaidOutcome(Result(Raid,P),L,{}));
    TestTrue(TEXT("Other dies"),S->CommitRaidOutcome(Result(Raid,Other,ETURaidPlayerOutcome::Dead),R,{}));
    TestEqual(TEXT("Only extracted owner retains equipment"),S->GetProfile()->Stash.Weapons.Num(),1);
    TestTrue(TEXT("Death cannot replenish with capture"),S->CaptureInitialKitForPlayer(Other,R)); TestEqual(TEXT("No second starter kit"),S->GetPlayerStash(Other).Weapons.Num(),0);
    const FGuid TrainingRaid=FGuid::NewGuid(); TestTrue(TEXT("Training deploy"),S->BeginDeployment(TrainingRaid,P,L,true));
    auto TrainingResult=Result(TrainingRaid,P); TrainingResult.bTraining=true;
    TestTrue(TEXT("Training ends"),S->CommitRaidOutcome(TrainingResult,L,{}));
    TestEqual(TEXT("Training no new rewards"),S->GetProfile()->CompletedOperations,1); TestEqual(TEXT("Training no duplicated item"),S->GetProfile()->Stash.Weapons.Num(),1);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPersistenceTransferTest, "TheUnit.Persistence.AcquisitionAndHandover", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPersistenceTransferTest::RunTest(const FString&)
{
    FPersistenceFixture F; auto* S=F.Store; const FGuid P=S->GetLocalPlayerId(), Other=FGuid::NewGuid(), Raid=FGuid::NewGuid(); auto L=Kit(P), R=Kit(Other);
    TestTrue(TEXT("Seed local"),S->CaptureInitialKit(L)); TestTrue(TEXT("Seed guest"),S->CaptureInitialKitForPlayer(Other,R));
    TestTrue(TEXT("Deploy local"),S->BeginDeployment(Raid,P,L,false)); TestTrue(TEXT("Deploy guest"),S->BeginDeployment(Raid,Other,R,false));
    L.LooseCartridges.Add(TEXT("556")); L.Magazines[1].Cartridges.RemoveAt(0);
    FTUItemLedger Acquired; auto Magazine=L.Magazines[1]; Magazine.OwnerId=Other; Magazine.WeaponId=R.Weapons[0].InstanceId; Acquired.Magazines.Add(Magazine);
    TestTrue(TEXT("Authority journals transferred magazine allowance"),S->RecordRaidAcquisition(Raid,Other,Acquired));
    L.Magazines.RemoveAt(1); R.Magazines.Add(Magazine);
    FTUItemInstance Item; Item.InstanceId=FGuid::NewGuid(); Item.OwnerId=P; Item.FoundInRaidId=Raid; Item.DefinitionId=TEXT("Intel");
    FTUItemLedger Loot; Loot.Items.Add(Item); TestTrue(TEXT("Authority records found item"),S->RecordRaidAcquisition(Raid,P,Loot)); L.Items.Add(Item);
    FTUTaskProgress Task; Task.PlayerId=P; Task.Definition.TaskId=TEXT("IntelHandover"); Task.Definition.Policy=ETUTaskPolicy::PhysicalHandover; Task.Definition.TargetId=TEXT("Intel"); Task.Definition.RequiredExtractId=TEXT("North");
    TestTrue(TEXT("Donor return excludes transferred mag"),S->CommitRaidOutcome(Result(Raid,P),L,{Task}));
    TestTrue(TEXT("Recipient returns acquired magazine once"),S->CommitRaidOutcome(Result(Raid,Other),R,{}));
    TestEqual(TEXT("Four magazines conserved across players"),S->GetProfile()->Stash.Magazines.Num(),4);
    S->InjectSaveFailure(3); TestFalse(TEXT("Handover save failure denies consumption"),S->CommitTaskHandover(P,TEXT("IntelHandover"),Item.InstanceId));
    TestEqual(TEXT("Item remains after failure"),S->GetProfile()->Stash.Items.Num(),1);
    S->InjectSaveFailure(0); TestTrue(TEXT("Retry handover"),S->CommitTaskHandover(P,TEXT("IntelHandover"),Item.InstanceId));
    TestTrue(TEXT("Duplicate handover acknowledged without second consumption"),S->CommitTaskHandover(P,TEXT("IntelHandover"),Item.InstanceId));
    auto* Reloaded=F.Reload(); TestEqual(TEXT("Consumed item absent after reload"),Reloaded->GetProfile()->Stash.Items.Num(),0); TestTrue(TEXT("Task complete after reload"),Reloaded->GetProfile()->Tasks[0].bCompleted);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPersistenceCrashLossTest, "TheUnit.Persistence.StartupAbandonsEscrow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPersistenceCrashLossTest::RunTest(const FString&)
{
    FPersistenceFixture F; auto* S=F.Store; const FGuid Player=S->GetLocalPlayerId(), Raid=FGuid::NewGuid(); auto L=Kit(Player);
    TestTrue(TEXT("Seed"),S->CaptureInitialKit(L)); TestTrue(TEXT("Deploy"),S->BeginDeployment(Raid,Player,L,false));
    L.Magazines[0].Cartridges.RemoveAt(0,11); // Unsaved live raid shot state lost at process death.
    auto* Restart=F.Reload(); Restart->InjectSaveFailure(3);
    TestFalse(TEXT("Recovery write failure blocks startup deployment"),Restart->RecoverUnresolvedDeployments()); TestTrue(TEXT("Blocked recovery exposed"),Restart->IsRecoveryBlocked());
    TestFalse(TEXT("Cannot redeploy while recovery blocked"),Restart->BeginDeployment(Raid,Player,L,false));
    Restart->InjectSaveFailure(0); TestTrue(TEXT("Recovery retry commits abandonment"),Restart->RecoverUnresolvedDeployments());
    TestEqual(TEXT("No predeploy weapon restoration"),Restart->GetPlayerStash(Player).Weapons.Num(),0);
    TestEqual(TEXT("No completed-operation reward"),Restart->GetProfile()->CompletedOperations,0);
    TestEqual(TEXT("Crash recorded as abandoned"),Restart->GetProfile()->Outcomes[0].Outcome,ETURaidPlayerOutcome::Abandoned);
    TestFalse(TEXT("Abandoned raid cannot later extract"),Restart->CommitRaidOutcome(Result(Raid,Player),L,{}));
    TestTrue(TEXT("One-time kit call harmless after loss"),Restart->CaptureInitialKit(L)); TestEqual(TEXT("No respawn kit mint"),Restart->GetPlayerStash(Player).Weapons.Num(),0);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPersistenceCheckpointTest, "TheUnit.Persistence.CumulativeCheckpointBatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPersistenceCheckpointTest::RunTest(const FString&)
{
    FPersistenceFixture F; auto* S=F.Store; const FGuid P=S->GetLocalPlayerId(), Other=FGuid::NewGuid(), Raid=FGuid::NewGuid(), Event=FGuid::NewGuid();
    auto L=Kit(P), R=Kit(Other); TestTrue(TEXT("Seed host"),S->CaptureInitialKit(L)); TestTrue(TEXT("Seed guest"),S->CaptureInitialKitForPlayer(Other,R));
    FTUDeploymentRecord A; A.RaidId=Raid; A.PlayerId=P; A.Escrow=L; FTUDeploymentRecord B=A; B.PlayerId=Other; B.Escrow=R;
    S->InjectSaveFailure(3); TestFalse(TEXT("Batch deployment denied on failed commit"),S->BeginDeploymentBatch({A,B}));
    TestEqual(TEXT("Neither participant moved on failure"),S->GetProfile()->Deployments.Num(),0); TestEqual(TEXT("Both kits remain owned"),S->GetProfile()->Stash.Weapons.Num(),2);
    S->InjectSaveFailure(0); TestTrue(TEXT("Atomic batch retry"),S->BeginDeploymentBatch({A,B})); TestEqual(TEXT("Both participants escrowed"),S->GetProfile()->Deployments.Num(),2);
    FTUTaskProgress T; T.PlayerId=P; T.Definition.TaskId=TEXT("Cumulative"); T.Definition.RequiredCount=3; T.Definition.Policy=ETUTaskPolicy::Cumulative; T.CommittedCount=1; T.CommittedSteps={1}; T.ProcessedEventIds={Event}; auto U=T; U.PlayerId=Other;
    for (int32 Failure=1;Failure<=3;++Failure)
    {
        S->InjectSaveFailure(Failure); TestFalse(TEXT("Checkpoint write failure"),S->CheckpointTasks(Raid,{T,U})); TestEqual(TEXT("Neither squad member receives partial durable credit"),S->GetProfile()->Tasks.Num(),0);
    }
    S->InjectSaveFailure(0); TestTrue(TEXT("Batch checkpoint retry"),S->CheckpointTasks(Raid,{T,U})); const int32 Sequence=S->GetProfile()->CommitSequence;
    TestTrue(TEXT("Exact event replay is idempotent"),S->CheckpointTasks(Raid,{T,U})); TestEqual(TEXT("Replay performs no additional save"),S->GetProfile()->CommitSequence,Sequence);
    auto Stale=T; Stale.CommittedCount=0; TestFalse(TEXT("Stale snapshot cannot erase credit"),S->CheckpointTasks(Raid,{Stale}));
    auto* Restart=F.Reload(); TestTrue(TEXT("Startup abandons both raids"),Restart->RecoverUnresolvedDeployments()); TestEqual(TEXT("Cumulative credit survives crash for both"),Restart->GetProfile()->Tasks.Num(),2);
    TestEqual(TEXT("Host cumulative credit preserved"),Restart->GetProfile()->Tasks[0].CommittedCount,1); TestEqual(TEXT("Guest cumulative credit preserved"),Restart->GetProfile()->Tasks[1].CommittedCount,1);
    TestFalse(TEXT("Closed raid cannot checkpoint"),Restart->CheckpointTasks(Raid,{T}));
    const FGuid TrainingRaid=FGuid::NewGuid(); TestTrue(TEXT("Training deployment"),Restart->BeginDeployment(TrainingRaid,P,L,true)); T.CommittedCount=2; T.CommittedSteps={2}; T.ProcessedEventIds.Add(FGuid::NewGuid());
    TestTrue(TEXT("Training checkpoint acknowledged locally"),Restart->CheckpointTasks(TrainingRaid,{T})); TestEqual(TEXT("Training cannot advance live task"),Restart->GetProfile()->Tasks[0].CommittedCount,1);
    FTUItemLedger TrainingLoot=L; FTUItemInstance Item; Item.InstanceId=FGuid::NewGuid(); Item.OwnerId=P; Item.DefinitionId=TEXT("TrainingIntel"); Item.FoundInRaidId=TrainingRaid; TrainingLoot.Items.Add(Item);
    auto Outcome=Result(TrainingRaid,P); Outcome.bTraining=true; TestTrue(TEXT("Training loot does not block sandbox resolution"),Restart->CommitRaidOutcome(Outcome,TrainingLoot,{T})); TestEqual(TEXT("Training loot not persisted"),Restart->GetProfile()->Stash.Items.Num(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPersistenceMigrationTest, "TheUnit.Persistence.MigrationAndBackup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPersistenceMigrationTest::RunTest(const FString&)
{
    FPersistenceFixture F; auto* Old=NewObject<UTUHideoutSaveGame>(); Old->SaveVersion=2; Old->PrimaryId=TEXT("LegacyPrimary"); Old->CompletedOperations=7; Old->GearBySlot.Add(ETUEquipmentSlot::Headwear,TEXT("Helmet"));
    TestTrue(TEXT("Write actual legacy archive"),UGameplayStatics::SaveGameToSlot(Old,F.Slot,0));
    auto* Migrated=F.Reload(); TestEqual(TEXT("Schema migrated"),Migrated->GetProfile()->SaveVersion,3); TestEqual(TEXT("Selected ID preserved"),Migrated->GetProfile()->PrimaryId,FName(TEXT("LegacyPrimary"))); TestEqual(TEXT("Progression preserved"),Migrated->GetProfile()->CompletedOperations,7); TestTrue(TEXT("Stable player assigned"),Migrated->GetLocalPlayerId().IsValid());
    TestTrue(TEXT("Save current"),Migrated->SaveProfile());
    TestTrue(TEXT("Simulate missing interrupted primary"),UGameplayStatics::DeleteGameInSlot(F.Slot,0));
    auto* Recovered=F.Reload(); TestEqual(TEXT("Backup recovers progression"),Recovered->GetProfile()->CompletedOperations,7); TestEqual(TEXT("Backup stable player"),Recovered->GetLocalPlayerId(),Migrated->GetLocalPlayerId());
    return true;
}
#endif
