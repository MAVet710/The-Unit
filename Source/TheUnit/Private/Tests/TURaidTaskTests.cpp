#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TUTaskRules.h"
#include "TU_GameMode.h"
#include "TU_ExtractionZone.h"
#include "TUHealthComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "TU_PlayerState.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_ModularOperatorCharacter.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
struct FRaidPersistenceFixture
{
    UWorld* World=nullptr;
    UGameInstance* GI=nullptr;
    UTUHideoutLifecycleSubsystem* Life=nullptr;
    FString Slot=TEXT("TU_Automation_Raid_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FRaidPersistenceFixture()
    {
        const UWorld::InitializationValues Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
        if(World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        GI=NewObject<UGameInstance>(); GI->AddToRoot(); Life=NewObject<UTUHideoutLifecycleSubsystem>(GI); Life->AddToRoot();
        Life->ConfigureTestSlot(Slot); Life->LoadProfile();
    }
    ~FRaidPersistenceFixture()
    {
        if(World)
        {
            World->DestroyWorld(false);
            if(GEngine) GEngine->DestroyWorldContext(World);
        }
        Life->RemoveFromRoot(); GI->RemoveFromRoot();
        for(const FString& Suffix:{FString(),FString(TEXT("_backup")),FString(TEXT("_journal"))}) UGameplayStatics::DeleteGameInSlot(Slot+Suffix,0);
    }
    ATU_GameMode* NewRaid()
    {
        ATU_GameMode* Mode=World->SpawnActor<ATU_GameMode>(); Mode->ConfigureTestLifecycle(Life); Mode->StartRaid(FGuid::NewGuid(),90.f,false); return Mode;
    }
    ATU_ArmedOperatorCharacter* NewGuest(const FGuid& Id)
    {
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ATU_ArmedOperatorCharacter* Pawn=World->SpawnActor<ATU_ModularOperatorCharacter>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
        Pawn->SpawnDefaultWeapon(); ATU_PlayerState* PS=World->SpawnActor<ATU_PlayerState>(); PS->PersistentPlayerId=Id; Pawn->SetPlayerState(PS); return Pawn;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUTaskPoliciesTest,"TheUnit.Execution.Tasks.PoliciesAndHandover",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUTaskPoliciesTest::RunTest(const FString&)
{
    const FGuid Player=FGuid::NewGuid(), Raid=FGuid::NewGuid();
    FTURaidOutcome Death; Death.PlayerId=Player; Death.RaidId=Raid; Death.Outcome=ETURaidPlayerOutcome::Dead;
    FTUTaskProgress C; C.PlayerId=Player; C.Definition.Condition=ETUTaskCondition::Eliminate; C.Definition.RequiredCount=3;
    TestTrue(TEXT("Eligible AI event earns cumulative credit"),FTUTaskRules::Record(C,Raid,ETUTaskCondition::Eliminate,NAME_None,1));
    FTUTaskRules::Resolve(C,Death); TestEqual(TEXT("Explicit cumulative credit survives death"),C.CommittedCount,1);
    FTUTaskProgress S; S.PlayerId=Player; S.Definition.Policy=ETUTaskPolicy::SameRaid;
    FTUTaskStep Visit; Visit.Condition=ETUTaskCondition::Visit; Visit.TargetId=TEXT("Location");
    FTUTaskStep Interact; Interact.Condition=ETUTaskCondition::Interact; Interact.TargetId=TEXT("Console");
    S.Definition.Steps={Visit,Interact};
    FTUTaskRules::Record(S,Raid,ETUTaskCondition::Visit,TEXT("Location"),1);
    TestFalse(TEXT("One heterogeneous step is incomplete"),S.bCompleted);
    FTUTaskRules::Resolve(S,Death); TestEqual(TEXT("Failed raid clears partial same-raid steps"),S.PendingCount,0);
    const FGuid NextRaid=FGuid::NewGuid();
    FTUTaskRules::Record(S,NextRaid,ETUTaskCondition::Interact,TEXT("Console"),1);
    TestFalse(TEXT("A new raid cannot reuse previous visit"),S.bCompleted);
    FTUTaskRules::Record(S,NextRaid,ETUTaskCondition::Visit,TEXT("Location"),1);
    TestTrue(TEXT("Both distinct steps in one raid complete task"),S.bCompleted);
    FTUTaskProgress E; E.PlayerId=Player; E.Definition.Policy=ETUTaskPolicy::ExtractRequired;
    E.Definition.Condition=ETUTaskCondition::Recover; E.Definition.TargetId=TEXT("Intel"); E.Definition.RequiredExtractId=TEXT("North");
    FTUTaskRules::Record(E,Raid,ETUTaskCondition::Recover,TEXT("Intel"),1);
    TestFalse(TEXT("Recovery alone is not extraction credit"),E.bCompleted);
    FTURaidOutcome Extract=Death; Extract.Outcome=ETURaidPlayerOutcome::Extracted; Extract.ExtractId=TEXT("South");
    FTUTaskRules::Resolve(E,Extract); TestFalse(TEXT("Wrong exit rejects named-exit task"),E.bCompleted);
    FTUTaskRules::Record(E,Raid,ETUTaskCondition::Recover,TEXT("Intel"),1); Extract.ExtractId=TEXT("North");
    FTUTaskRules::Resolve(E,Extract); TestTrue(TEXT("Valid named extraction commits credit"),E.bCompleted);
    FTUTaskProgress H; H.PlayerId=Player; H.Definition.Policy=ETUTaskPolicy::PhysicalHandover;
    H.Definition.TargetId=TEXT("Intel"); H.Definition.RequiredExtractId=TEXT("North");
    FTUItemLedger L; FTUItemInstance Item; Item.InstanceId=FGuid::NewGuid(); Item.OwnerId=Player;
    Item.DefinitionId=TEXT("Intel"); Item.FoundInRaidId=Raid; L.Items.Add(Item);
    TestFalse(TEXT("Unextracted item is ineligible"),FTUTaskRules::Handover(H,L,Item.InstanceId));
    L.Items[0].bExtracted=true; L.Items[0].ExtractedAtId=TEXT("South");
    TestFalse(TEXT("Handover requires actual named-exit provenance"),FTUTaskRules::Handover(H,L,Item.InstanceId));
    L.Items[0].ExtractedAtId=TEXT("North");
    TestTrue(TEXT("Actual eligible item consumed"),FTUTaskRules::Handover(H,L,Item.InstanceId));
    TestEqual(TEXT("Concrete inventory location changes"),L.Items[0].Location,ETUItemLocation::Consumed);
    TestFalse(TEXT("Duplicate handover cannot double spend"),FTUTaskRules::Handover(H,L,Item.InstanceId));

    FTUTaskProgress D; D.PlayerId=Player; D.Definition.Policy=ETUTaskPolicy::SameRaid;
    D.Definition.Condition=ETUTaskCondition::Destroy; D.Definition.TargetId=TEXT("SupplyCache");
    TestTrue(TEXT("Destroy event is a first-class task condition"),FTUTaskRules::Record(D,Raid,ETUTaskCondition::Destroy,TEXT("SupplyCache"),1));
    TestTrue(TEXT("Single cache destruction completes same-raid task"),D.bCompleted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTURecoverPhysicalExtractionTest,"TheUnit.Execution.Tasks.RecoveredItemMustRemainCarried",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTURecoverPhysicalExtractionTest::RunTest(const FString&)
{
    FRaidPersistenceFixture F; const FGuid Player=FGuid::NewGuid();
    ATU_GameMode* Mode=F.NewRaid(); ATU_ArmedOperatorCharacter* Pawn=F.NewGuest(Player);
    if(!TestEqual(TEXT("Participant ready"),Mode->RegisterParticipant(Pawn),Player)) return false;
    FTUTaskDefinition Escort; Escort.TaskId=TEXT("EscortPackage"); Escort.Policy=ETUTaskPolicy::ExtractRequired;
    Escort.Condition=ETUTaskCondition::Recover; Escort.TargetId=TEXT("EscortPackage"); Escort.RequiredExtractId=TEXT("NorthRoad");
    TestTrue(TEXT("Escort task accepted"),Mode->AddTask(Pawn,Escort));
    const FGuid PackageId=FGuid::NewGuid();
    TestTrue(TEXT("Physical package recovered"),Mode->RecoverItem(Pawn,TEXT("EscortPackage"),PackageId));
    FTUItemLedger Dropped=Pawn->ExportItemLedger();
    FTUItemInstance* Package=Dropped.Items.FindByPredicate([&](const FTUItemInstance& I){return I.InstanceId==PackageId;});
    if(!TestNotNull(TEXT("Recovered package exists in carried ledger"),Package)) return false;
    Package->Location=ETUItemLocation::Ground;
    TestTrue(TEXT("Dropped state imports"),Pawn->ImportItemLedger(Dropped));
    TestTrue(TEXT("Authoritative raid ledger sees dropped state"),Mode->SetParticipantLedger(Pawn,Dropped));
    ATU_ExtractionZone* Exit=F.World->SpawnActor<ATU_ExtractionZone>(); Exit->ExtractId=TEXT("NorthRoad"); Exit->HoldSeconds=0.f; Exit->Capacity=1;
    TestTrue(TEXT("Player may extract after dropping mission package"),Mode->BeginExtraction(Pawn,Exit));
    Mode->AdvanceRaidTime(.01f);
    const FTURaidParticipantState* P=Mode->FindParticipant(Pawn);
    const FTUTaskProgress* Task=P?P->Tasks.FindByPredicate([](const FTUTaskProgress& T){return T.Definition.TaskId==TEXT("EscortPackage");}):nullptr;
    TestTrue(TEXT("Player extracted"),P && P->Outcome==ETURaidPlayerOutcome::Extracted);
    TestTrue(TEXT("Escort task still exists"),Task!=nullptr);
    TestFalse(TEXT("Dropped package cannot complete escort mission"),Task && Task->bCompleted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTURaidActorRulesTest,"TheUnit.Execution.Raid.CountdownCancellationDeathExpiry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTURaidActorRulesTest::RunTest(const FString&)
{
    const UWorld::InitializationValues Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    if(!TestNotNull(TEXT("Transient runtime world"),World)) return false;
    ATU_GameMode* Mode=World->SpawnActor<ATU_GameMode>(); Mode->StartRaid(FGuid::NewGuid(),30.f,true);
    ACharacter* A=World->SpawnActor<ACharacter>(); ACharacter* B=World->SpawnActor<ACharacter>();
    const FGuid AId=Mode->RegisterParticipant(A), BId=Mode->RegisterParticipant(B);
    TestTrue(TEXT("Participants have distinct stable raid identities"),AId.IsValid() && BId.IsValid() && AId!=BId);
    ATU_ExtractionZone* Zone=World->SpawnActor<ATU_ExtractionZone>(); Zone->HoldSeconds=3.f;
    FTUTaskDefinition Unfinished; Unfinished.TaskId=TEXT("Unfinished"); Unfinished.RequiredCount=20; Mode->AddTask(A,Unfinished);
    TestTrue(TEXT("Ordinary exit accepts unfinished tasks"),Mode->BeginExtraction(A,Zone));
    TestFalse(TEXT("Direct result cannot bypass countdown"),Mode->ResolvePlayerOutcome(A,ETURaidPlayerOutcome::Extracted,Zone->ExtractId));
    Mode->AdvanceRaidTime(1.f); A->SetActorLocation(FVector(5000,0,0)); Mode->AdvanceRaidTime(1.f);
    TestFalse(TEXT("Leaving geometry cancels countdown"),Mode->FindParticipant(A)->bExtracting);
    A->SetActorLocation(FVector::ZeroVector); TestTrue(TEXT("Re-enter starts fresh countdown"),Mode->BeginExtraction(A,Zone));
    Mode->AdvanceRaidTime(2.f); TestEqual(TEXT("No accumulated partial timer"),Mode->FindParticipant(A)->Outcome,ETURaidPlayerOutcome::Active);
    Mode->AdvanceRaidTime(1.f); TestEqual(TEXT("Valid countdown extracts only A"),Mode->FindParticipant(A)->Outcome,ETURaidPlayerOutcome::Extracted);
    TestEqual(TEXT("B remains active"),Mode->FindParticipant(B)->Outcome,ETURaidPlayerOutcome::Active);
    TestFalse(TEXT("Duplicate terminal transition rejected"),Mode->ResolvePlayerOutcome(A,ETURaidPlayerOutcome::Dead));
    TestTrue(TEXT("B starts own countdown"),Mode->BeginExtraction(B,Zone));
    Mode->ResolvePlayerOutcome(B,ETURaidPlayerOutcome::Dead); Mode->AdvanceRaidTime(4.f);
    TestEqual(TEXT("Death never becomes surviving extraction"),Mode->FindParticipant(B)->Outcome,ETURaidPlayerOutcome::Dead);
    ACharacter* C=World->SpawnActor<ACharacter>(); Mode->RegisterParticipant(C);
    Mode->ResolvePlayerOutcome(C,ETURaidPlayerOutcome::DisconnectedPendingResolution); Mode->AdvanceRaidTime(30.f);
    TestEqual(TEXT("Disconnected unresolved participant expires without reward"),Mode->FindParticipant(C)->Outcome,ETURaidPlayerOutcome::TimedOut);
    World->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTURaidGuestReentryTest,"TheUnit.Execution.Raid.GuestHydrationAcrossRaids",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTURaidGuestReentryTest::RunTest(const FString&)
{
    FRaidPersistenceFixture F; const FGuid Guest=FGuid::NewGuid();
    ATU_GameMode* First=F.NewRaid(); ATU_ArmedOperatorCharacter* A=F.NewGuest(Guest);
    TestEqual(TEXT("First guest registered under stable identity"),First->RegisterParticipant(A),Guest);
    FTUItemLedger Before=A->ExportItemLedger();
    if(!TestTrue(TEXT("Authored initial kit exists"),Before.Weapons.Num()>0)) return false;
    Before.Weapons[0].ConditionNormalized=.37f; TestTrue(TEXT("Wear actual actor item"),A->ImportItemLedger(Before));
    ATU_ExtractionZone* Exit=F.World->SpawnActor<ATU_ExtractionZone>(); Exit->HoldSeconds=0.f;
    TestTrue(TEXT("Guest extraction starts"),First->BeginExtraction(A,Exit)); First->AdvanceRaidTime(.01f);
    TestEqual(TEXT("Guest extraction committed"),First->FindParticipant(A)->Outcome,ETURaidPlayerOutcome::Extracted);
    ATU_GameMode* Second=F.NewRaid(); ATU_ArmedOperatorCharacter* B=F.NewGuest(Guest);
    TestEqual(TEXT("Same guest returns without minting defaults"),Second->RegisterParticipant(B),Guest);
    const FTUItemLedger Restored=B->ExportItemLedger();
    TestEqual(TEXT("Same weapon identities preserved across next raid"),Restored.Weapons.Num(),Before.Weapons.Num());
    if(!Restored.Weapons.IsEmpty())
    {
        TestEqual(TEXT("First weapon ID unchanged"),Restored.Weapons[0].InstanceId,Before.Weapons[0].InstanceId);
        TestEqual(TEXT("Actual saved condition hydrated"),Restored.Weapons[0].ConditionNormalized,.37f);
    }
    TestTrue(TEXT("Second raid guest dies"),Second->ResolvePlayerOutcome(B,ETURaidPlayerOutcome::Dead));
    ATU_GameMode* Third=F.NewRaid(); ATU_ArmedOperatorCharacter* C=F.NewGuest(Guest);
    TestEqual(TEXT("Third raid admits same empty owner"),Third->RegisterParticipant(C),Guest);
    TestTrue(TEXT("Death loss overrides newly spawned default weapons"),C->ExportItemLedger().Weapons.IsEmpty());
    ATU_PlayerState* Destination=F.World->SpawnActor<ATU_PlayerState>(); C->GetPlayerState<ATU_PlayerState>()->CopyProperties(Destination);
    TestEqual(TEXT("Seamless PlayerState preserves identity"),Destination->PersistentPlayerId,Guest);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTURaidCheckpointFailureTest,"TheUnit.Execution.Raid.TaskCheckpointFailureAndOrphanRetry",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTURaidCheckpointFailureTest::RunTest(const FString&)
{
    FRaidPersistenceFixture F; const FGuid Guest=FGuid::NewGuid();
    ATU_GameMode* Mode=F.NewRaid(); ATU_ArmedOperatorCharacter* Pawn=F.NewGuest(Guest);
    if(!TestEqual(TEXT("Participant ready"),Mode->RegisterParticipant(Pawn),Guest)) return false;
    FTUTaskDefinition C; C.TaskId=TEXT("Checkpoint"); C.Condition=ETUTaskCondition::Interact; C.RequiredCount=3;
    Mode->AddTask(Pawn,C); const FGuid Event=FGuid::NewGuid();
    F.Life->InjectSaveFailure(1);
    TestFalse(TEXT("Failed cumulative checkpoint rejects event"),Mode->RecordUniqueTaskEvent(Pawn,Event,ETUTaskCondition::Interact,NAME_None));
    TestEqual(TEXT("Failed checkpoint publishes no live progress"),Mode->FindParticipant(Pawn)->Tasks[0].CommittedCount,0);
    F.Life->InjectSaveFailure(0);
    TestTrue(TEXT("Same event remains retryable after failed save"),Mode->RecordUniqueTaskEvent(Pawn,Event,ETUTaskCondition::Interact,NAME_None));
    TestFalse(TEXT("Committed duplicate event is harmless"),Mode->RecordUniqueTaskEvent(Pawn,Event,ETUTaskCondition::Interact,NAME_None));
    TestEqual(TEXT("Exactly one credit persisted"),F.Life->GetProfile()->Tasks[0].CommittedCount,1);
    FTUTaskDefinition S; S.TaskId=TEXT("SurviveOrphan"); S.Policy=ETUTaskPolicy::ExtractRequired; S.Condition=ETUTaskCondition::Survive;
    Mode->AddTask(Pawn,S);
    ATU_ExtractionZone* Exit=F.World->SpawnActor<ATU_ExtractionZone>(); Exit->HoldSeconds=0.f; Exit->Capacity=1;
    TestTrue(TEXT("Begin real extraction"),Mode->BeginExtraction(Pawn,Exit));
    F.Life->InjectSaveFailure(1); Mode->AdvanceRaidTime(.01f);
    TestTrue(TEXT("Failed outcome is reserved"),Mode->FindParticipant(Pawn)->bSavePending);
    TestFalse(TEXT("Frozen outcome refuses unrelated task mutations"),Mode->AddTask(Pawn,FTUTaskDefinition()));
    TestFalse(TEXT("Logout cannot replace reserved extracted candidate"),Mode->ResolvePlayerOutcome(Pawn,ETURaidPlayerOutcome::DisconnectedPendingResolution));
    TestFalse(TEXT("Pending pawn has no damage exposure"),Pawn->CanBeDamaged());
    Pawn->Destroy(); F.Life->InjectSaveFailure(0); Mode->AdvanceRaidTime(1.1f);
    const FTURaidParticipantState* P=Mode->FindParticipantById(Guest);
    TestEqual(TEXT("Destroyed pawn retries its reserved extraction"),P->Outcome,ETURaidPlayerOutcome::Extracted);
    const FTUTaskProgress* Survival=P->Tasks.FindByPredicate([](const FTUTaskProgress& T){return T.Definition.TaskId==TEXT("SurviveOrphan");});
    TestTrue(TEXT("Orphan receives the same survival event as live retry"),Survival && Survival->bCompleted);
    TestEqual(TEXT("Exit capacity reserved exactly once"),Exit->UsedCapacity,1);
    TestEqual(TEXT("Single durable terminal outcome"),F.Life->GetProfile()->Outcomes.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTURaidIrreversibleEventRetryTest,"TheUnit.Execution.Raid.PendingKillKeepsOriginalRecipients",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTURaidIrreversibleEventRetryTest::RunTest(const FString&)
{
    FRaidPersistenceFixture F; ATU_GameMode* Mode=F.NewRaid();
    ATU_ArmedOperatorCharacter* Players[3];
    FTUTaskDefinition Task; Task.TaskId=TEXT("QueuedKill"); Task.Condition=ETUTaskCondition::Eliminate;
    Task.TargetId=TEXT("Guard"); Task.CreditOwner=ETUTaskCreditOwner::EligiblePresentSquad; Task.RequiredCount=3;
    for(int32 I=0;I<3;++I)
    {
        Players[I]=F.NewGuest(FGuid::NewGuid()); Players[I]->SetActorLocation(FVector(I==2?5000.f:I*100.f,0,0));
        TestTrue(TEXT("Guest registered"),Mode->RegisterParticipant(Players[I]).IsValid()); Mode->AddTask(Players[I],Task);
    }
    F.Life->InjectSaveFailure(1);
    Mode->RecordTaskEvent(Players[0],ETUTaskCondition::Eliminate,TEXT("Guard"),1,FGuid::NewGuid());
    Players[1]->SetActorLocation(FVector(5000,0,0)); Players[2]->SetActorLocation(FVector(100,0,0));
    Players[0]->FindComponentByClass<UTUHealthComponent>()->ApplyRegionalDamage(ETUBodyRegion::Head,100000.f);
    TestFalse(TEXT("Death result waits for earlier irreversible task event"),Mode->ResolvePlayerOutcome(Players[0],ETURaidPlayerOutcome::Dead));
    F.Life->InjectSaveFailure(0); Mode->AdvanceRaidTime(2.1f);
    TestEqual(TEXT("Killer's earlier event survives subsequent death"),Mode->FindParticipant(Players[0])->Tasks[0].CommittedCount,1);
    TestEqual(TEXT("Originally present teammate retains eligibility after leaving"),Mode->FindParticipant(Players[1])->Tasks[0].CommittedCount,1);
    TestEqual(TEXT("New arrival cannot receive retroactive squad credit"),Mode->FindParticipant(Players[2])->Tasks[0].CommittedCount,0);
    TestEqual(TEXT("Actual health death resolves after checkpoint retry"),Mode->FindParticipant(Players[0])->Outcome,ETURaidPlayerOutcome::Dead);
    return true;
}
#endif
