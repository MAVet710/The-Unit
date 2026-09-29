#include "TU_GameMode.h"
#include "TU_ModularOperatorCharacter.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_PlayerController.h"
#include "TU_PlayerState.h"
#include "TU_RaidHUD.h"
#include "TU_WorldLighting.h"
#include "TU_ExtractionZone.h"
#include "TU_HideoutGameMode.h"
#include "TUHealthComponent.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "TUTaskRules.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TUExecutionSmokeActor.h"
#include "TUHandlingCapture.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace
{
TArray<FTUTaskProgress> BuildOutcomeTasks(const TArray<FTUTaskProgress>& Existing,const FTURaidOutcome& Result,const FTUItemLedger& Ledger)
{
    TArray<FTUTaskProgress> Tasks=Existing;
    if(Result.Outcome==ETURaidPlayerOutcome::Extracted)
    {
        const auto HasPhysicalRecover=[&](FName DefinitionId)
        {
            if(DefinitionId.IsNone()) return true;
            return Ledger.Items.ContainsByPredicate([&](const FTUItemInstance& I)
            {
                return I.OwnerId==Result.PlayerId && I.DefinitionId==DefinitionId && I.FoundInRaidId==Result.RaidId &&
                    (I.Location==ETUItemLocation::Carried || I.Location==ETUItemLocation::InHand);
            });
        };
        for(FTUTaskProgress& T:Tasks)
        {
            if(T.Definition.Policy==ETUTaskPolicy::ExtractRequired && T.PendingRaidId==Result.RaidId)
            {
                if(T.Definition.Steps.IsEmpty())
                {
                    if(T.Definition.Condition==ETUTaskCondition::Recover && !HasPhysicalRecover(T.Definition.TargetId))
                    { T.PendingSteps.Init(0,1); T.PendingCount=0; }
                }
                else if(T.PendingSteps.Num()==T.Definition.Steps.Num())
                {
                    for(int32 Index=0;Index<T.Definition.Steps.Num();++Index)
                    {
                        const FTUTaskStep& Step=T.Definition.Steps[Index];
                        if(Step.Condition==ETUTaskCondition::Recover && !HasPhysicalRecover(Step.TargetId))
                            T.PendingSteps[Index]=0;
                    }
                    T.PendingCount=0; for(int32 Count:T.PendingSteps) T.PendingCount+=Count;
                }
            }
            FTUTaskRules::Record(T,Result.RaidId,ETUTaskCondition::Survive,Result.ExtractId,1);
        }
    }
    for(FTUTaskProgress& T:Tasks) FTUTaskRules::Resolve(T,Result);
    return Tasks;
}
}

ATU_GameMode::ATU_GameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ATU_ModularOperatorCharacter::StaticClass();
    PlayerControllerClass = ATU_PlayerController::StaticClass();
    GameStateClass = ATU_GameState::StaticClass();
    PlayerStateClass = ATU_PlayerState::StaticClass();
    HUDClass = ATU_RaidHUD::StaticClass();
    bUseSeamlessTravel = true;
}
UTUHideoutLifecycleSubsystem* ATU_GameMode::GetLifecycle() const
{
    return LifecycleOverride ? LifecycleOverride.Get() :
        (GetGameInstance() ? GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>() : nullptr);
}
void ATU_GameMode::StartPlay()
{
    Super::StartPlay();
    FString HandlingCaptureId;
    if (HasAuthority() && GetWorld() && FParse::Value(FCommandLine::Get(), TEXT("TUHandlingCapture="), HandlingCaptureId))
        GetWorld()->SpawnActor<ATUHandlingCapture>();
    if(HasAuthority() && GetWorld()) GetWorld()->SpawnActor<ATU_WorldLighting>();
    FString SmokeMode;
    const bool Smoke=FParse::Value(FCommandLine::Get(),TEXT("TUExecutionSmoke="),SmokeMode);
    if (IsA<ATU_HideoutGameMode>())
    {
        if(Smoke && GetWorld()) GetWorld()->SpawnActor<ATUExecutionSmokeActor>();
        return;
    }
    UTUHideoutLifecycleSubsystem* Life=GetLifecycle();
    FGuid Id=Life?Life->GetActiveRaidId():FGuid();
    StartRaid(Id.IsValid()?Id:FGuid::NewGuid(),RaidDurationSeconds,bTrainingRaid);
    if(Smoke && GetWorld()) GetWorld()->SpawnActor<ATUExecutionSmokeActor>();
}
void ATU_GameMode::StartRaid(FGuid Id,float Duration,bool Training)
{
    if (!HasAuthority() || bRaidStarted || !Id.IsValid() || Duration<=0.f || !FMath::IsFinite(Duration)) return;
    RaidState=GetGameState<ATU_GameState>();
    if (!RaidState && GetWorld()) RaidState=GetWorld()->SpawnActor<ATU_GameState>();
    if (!RaidState) return;
    bRaidStarted=true; bTrainingRaid=Training; RaidDurationSeconds=Duration; ElapsedTime=0.f;
    RaidState->RaidId=Id; RaidState->RaidEndTime=Duration; RaidState->bTraining=Training;
}
FGuid ATU_GameMode::RegisterParticipant(APawn* Pawn)
{
    if (!HasAuthority() || !Pawn || !bRaidStarted) return FGuid();
    if (const FGuid* Existing=PawnIds.Find(Pawn)) return *Existing;
    ATU_PlayerState* PS=Pawn->GetPlayerState<ATU_PlayerState>();
    UTUHideoutLifecycleSubsystem* Life=GetLifecycle();
    FGuid Id=PS?PS->PersistentPlayerId:FGuid();
    const APlayerController* PC=Cast<APlayerController>(Pawn->GetController());
    if (!Id.IsValid())
    {
        if(Life && PC && PC->IsLocalController()) Id=Life->GetLocalPlayerId();
        else if(Life && PS)
        {
            const FString Key=PS->GetUniqueId().IsValid()?PS->GetUniqueId().ToString():PS->GetPlayerName();
            Id=Life->GetOrCreatePlayerId(Key);
        }
        else Id=FGuid::NewGuid();
    }
    if(!Id.IsValid()) return FGuid();
    FTURaidParticipantState* Returning=RaidState->Participants.FindByPredicate([&](const FTURaidParticipantState& P){return P.PlayerId==Id;});
    if(Returning)
    {
        if(Returning->Outcome!=ETURaidPlayerOutcome::DisconnectedPendingResolution || Returning->bSavePending) return FGuid();
        FTUItemLedger Ledger=Ledgers.FindRef(Id);
        if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn))
            if(!Armed->ImportItemLedger(Ledger)) return FGuid();
        for(auto It=PawnIds.CreateIterator();It;++It) if(It.Value()==Id) It.RemoveCurrent();
        PawnIds.Add(Pawn,Id); Returning->Outcome=ETURaidPlayerOutcome::Active; Returning->bExtracting=false;
        if(PS) PS->PersistentPlayerId=Id;
        SyncPlayerState(Pawn);
        return Id;
    }
    FTUItemLedger Ledger;
    if (ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Ledger=Armed->ExportItemLedger();
    // A returning guest's authored defaults are presentation only. Restore the owner's exact durable kit,
    // including an empty kit after death, before this raid can claim any item ownership.
    if(Life && Life->GetProfile())
    {
        const FTUDeploymentRecord* Existing=Life->GetProfile()->Deployments.FindByPredicate(
            [&](const FTUDeploymentRecord& D){return D.PlayerId==Id && D.RaidId==RaidState->RaidId && !D.bResolved;});
        if(Existing)
        {
            if(Existing->bTraining!=bTrainingRaid) return FGuid();
            Ledger=Existing->Escrow;
        }
        else if(!bTrainingRaid && Life->GetProfile()->InitializedPlayers.Contains(Id)) Ledger=Life->GetPlayerStash(Id);
    }
    for(FWeaponInstanceState& W:Ledger.Weapons) W.OwnerId=Id;
    for(FTUMagazineInstance& M:Ledger.Magazines) M.OwnerId=Id;
    for(FTUItemInstance& I:Ledger.Items) I.OwnerId=Id;
    if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn))
        if(!Armed->ImportItemLedger(Ledger)) return FGuid();
    if (Life)
    {
        const bool AlreadyDeployed=Life->GetProfile() && Life->GetProfile()->Deployments.ContainsByPredicate(
            [&](const FTUDeploymentRecord& D){return D.PlayerId==Id && D.RaidId==RaidState->RaidId && !D.bResolved;});
        if (!AlreadyDeployed && ((!bTrainingRaid && !Life->CaptureInitialKitForPlayer(Id,Ledger)) ||
            !Life->BeginDeployment(RaidState->RaidId,Id,Ledger,bTrainingRaid))) return FGuid();
    }
    else if (!bTrainingRaid) return FGuid();
    FTURaidParticipantState P; P.PlayerId=Id;
    if (Life && Life->GetProfile() && !bTrainingRaid)
        for (const FTUTaskProgress& T:Life->GetProfile()->Tasks) if(T.PlayerId==Id) P.Tasks.Add(T);
    PawnIds.Add(Pawn,Id); Ledgers.Add(Id,Ledger); RaidState->Participants.Add(P);
    if(PS) PS->PersistentPlayerId=Id;
    for(const FTUTaskDefinition& Definition:DefaultTasks) AddTask(Pawn,Definition);
    SyncPlayerState(Pawn);
    return Id;
}
FTURaidParticipantState* ATU_GameMode::MutableParticipant(APawn* Pawn)
{
    const FGuid* Id=PawnIds.Find(Pawn);
    return Id && RaidState?RaidState->Participants.FindByPredicate([&](const FTURaidParticipantState& P){return P.PlayerId==*Id;}):nullptr;
}
const FTURaidParticipantState* ATU_GameMode::FindParticipant(APawn* Pawn) const
{
    return const_cast<ATU_GameMode*>(this)->MutableParticipant(Pawn);
}
const FTURaidParticipantState* ATU_GameMode::FindParticipantById(const FGuid& PlayerId) const
{
    return RaidState?RaidState->Participants.FindByPredicate([&](const FTURaidParticipantState& P){return P.PlayerId==PlayerId;}):nullptr;
}
void ATU_GameMode::SyncPlayerState(APawn* Pawn)
{
    if (const FTURaidParticipantState* P=FindParticipant(Pawn))
        if (ATU_PlayerState* PS=Pawn->GetPlayerState<ATU_PlayerState>()) { PS->TaskProgress=P->Tasks; PS->ForceNetUpdate(); }
    if (RaidState) RaidState->ForceNetUpdate();
}
bool ATU_GameMode::SetParticipantLedger(APawn* Pawn,const FTUItemLedger& Ledger)
{
    const FTURaidParticipantState* P=FindParticipant(Pawn);
    if(!HasAuthority() || !P || P->Outcome!=ETURaidPlayerOutcome::Active || P->bSavePending) return false;
    for(const FTUItemInstance& I:Ledger.Items) if(I.OwnerId!=P->PlayerId) return false;
    for(const FWeaponInstanceState& W:Ledger.Weapons) if(W.OwnerId!=P->PlayerId) return false;
    for(const FTUMagazineInstance& M:Ledger.Magazines) if(M.OwnerId!=P->PlayerId) return false;
    Ledgers.Add(P->PlayerId,Ledger); return true;
}
const FTUItemLedger* ATU_GameMode::GetParticipantLedger(APawn* Pawn) const
{
    const FTURaidParticipantState* P=FindParticipant(Pawn);
    return P?Ledgers.Find(P->PlayerId):nullptr;
}
bool ATU_GameMode::AddTask(APawn* Pawn,const FTUTaskDefinition& Definition)
{
    FTURaidParticipantState* P=MutableParticipant(Pawn);
    if (!HasAuthority() || !P || P->bSavePending || Definition.TaskId.IsNone() || Definition.RequiredCount<=0 || P->Outcome!=ETURaidPlayerOutcome::Active ||
        P->Tasks.ContainsByPredicate([&](const FTUTaskProgress& T){return T.Definition.TaskId==Definition.TaskId;})) return false;
    FTUTaskProgress T; T.Definition=Definition; T.PlayerId=P->PlayerId; P->Tasks.Add(T); SyncPlayerState(Pawn); return true;
}
TArray<FGuid> ATU_GameMode::GetTaskRecipients(APawn* Pawn) const
{
    TArray<FGuid> Recipients;
    if(!Pawn) return Recipients;
    for(const auto& Entry:PawnIds)
    {
        APawn* Recipient=Entry.Key.Get(); const FTURaidParticipantState* R=FindParticipant(Recipient);
        if(!Recipient || !R || R->Outcome!=ETURaidPlayerOutcome::Active || R->bSavePending) continue;
        if(const UTUHealthComponent* Health=Recipient->FindComponentByClass<UTUHealthComponent>()) if(Health->IsDead()) continue;
        if(Recipient==Pawn || FVector::DistSquared(Recipient->GetActorLocation(),Pawn->GetActorLocation())<=FMath::Square(1500.f)) Recipients.Add(R->PlayerId);
    }
    return Recipients;
}
void ATU_GameMode::RecordTaskEvent(APawn* Pawn,ETUTaskCondition Condition,FName TargetId,int32 Amount,FGuid EventId)
{
    if(!EventId.IsValid()) EventId=FGuid::NewGuid();
    const FTURaidParticipantState* P=FindParticipant(Pawn);
    if(!HasAuthority() || !P || P->Outcome!=ETURaidPlayerOutcome::Active || P->bSavePending || Amount<=0 ||
        SeenTaskEvents.FindOrAdd(P->PlayerId).Contains(EventId) || PendingTaskEvents.ContainsByPredicate(
            [&](const FPendingTaskEvent& E){return E.PlayerId==P->PlayerId && E.EventId==EventId;})) return;
    const TArray<FGuid> Recipients=GetTaskRecipients(Pawn);
    if(ApplyTaskEvent(P->PlayerId,EventId,Condition,TargetId,Amount,Recipients)) return;
    FPendingTaskEvent Event; Event.PlayerId=P->PlayerId; Event.Recipients=Recipients; Event.EventId=EventId; Event.Condition=Condition;
    Event.TargetId=TargetId; Event.Amount=Amount; Event.RetryAt=ElapsedTime+2.f; PendingTaskEvents.Add(Event);
}
bool ATU_GameMode::RecordUniqueTaskEvent(APawn* Pawn,const FGuid& EventId,ETUTaskCondition Condition,FName TargetId,int32 Amount)
{
    FTURaidParticipantState* P=MutableParticipant(Pawn);
    if(!HasAuthority() || !P || !EventId.IsValid() || Amount<=0 || P->Outcome!=ETURaidPlayerOutcome::Active || P->bSavePending ||
        SeenTaskEvents.FindOrAdd(P->PlayerId).Contains(EventId)) return false;
    return ApplyTaskEvent(P->PlayerId,EventId,Condition,TargetId,Amount,GetTaskRecipients(Pawn));
}
bool ATU_GameMode::ApplyTaskEvent(const FGuid& PlayerId,const FGuid& EventId,ETUTaskCondition Condition,FName TargetId,int32 Amount,const TArray<FGuid>& Recipients)
{
    if(!HasAuthority() || !RaidState || !PlayerId.IsValid() || !EventId.IsValid() || Amount<=0) return false;
    TMap<FGuid,TArray<FTUTaskProgress>> CandidateTasks;
    TArray<FTUTaskProgress> Checkpoint;
    for(const FGuid& Recipient:Recipients)
    {
        const FTURaidParticipantState* R=FindParticipantById(Recipient);
        if(!R || R->bSavePending || (R->Outcome!=ETURaidPlayerOutcome::Active && R->Outcome!=ETURaidPlayerOutcome::DisconnectedPendingResolution)) return false;
        TArray<FTUTaskProgress> Tasks=R->Tasks;
        for(FTUTaskProgress& T:Tasks)
        {
            if(Recipient!=PlayerId && T.Definition.CreditOwner!=ETUTaskCreditOwner::EligiblePresentSquad) continue;
            if(T.ProcessedEventIds.Contains(EventId)) continue;
            if(FTUTaskRules::Record(T,RaidState->RaidId,Condition,TargetId,Amount)) T.ProcessedEventIds.Add(EventId);
        }
        Checkpoint.Append(Tasks);
        CandidateTasks.Add(R->PlayerId,MoveTemp(Tasks));
    }
    if(UTUHideoutLifecycleSubsystem* Life=GetLifecycle())
        if(!Life->CheckpointTasks(RaidState->RaidId,Checkpoint)) return false;
    SeenTaskEvents.FindOrAdd(PlayerId).Add(EventId);
    for(FTURaidParticipantState& R:RaidState->Participants)
        if(TArray<FTUTaskProgress>* Tasks=CandidateTasks.Find(R.PlayerId)) R.Tasks=MoveTemp(*Tasks);
    for(const auto& Entry:PawnIds) if(APawn* Recipient=Entry.Key.Get()) SyncPlayerState(Recipient);
    RaidState->ForceNetUpdate(); return true;
}
bool ATU_GameMode::FlushPendingTaskEvents(const FGuid& RequiredPlayerId)
{
    bool AllCommitted=true;
    for(int32 Index=PendingTaskEvents.Num()-1;Index>=0;--Index)
    {
        FPendingTaskEvent& Event=PendingTaskEvents[Index];
        if(RequiredPlayerId.IsValid() && Event.PlayerId!=RequiredPlayerId && !Event.Recipients.Contains(RequiredPlayerId)) continue;
        if(SeenTaskEvents.FindOrAdd(Event.PlayerId).Contains(Event.EventId)) { PendingTaskEvents.RemoveAtSwap(Index); continue; }
        if(ElapsedTime<Event.RetryAt) { AllCommitted=false; continue; }
        if(ApplyTaskEvent(Event.PlayerId,Event.EventId,Event.Condition,Event.TargetId,Event.Amount,Event.Recipients)) PendingTaskEvents.RemoveAtSwap(Index);
        else { Event.RetryAt=ElapsedTime+2.f; AllCommitted=false; }
    }
    return AllCommitted;
}
bool ATU_GameMode::RecoverItem(APawn* Pawn,FName DefinitionId,const FGuid& ItemId)
{
    FTURaidParticipantState* P=MutableParticipant(Pawn);
    if(!HasAuthority() || !P || !ItemId.IsValid() || DefinitionId.IsNone() || P->Outcome!=ETURaidPlayerOutcome::Active || P->bSavePending) return false;
    for(const auto& Entry:Ledgers)
        if(Entry.Value.Items.ContainsByPredicate([&](const FTUItemInstance& I){return I.InstanceId==ItemId;})) return false;
    FTUItemLedger Ledger=Ledgers.FindChecked(P->PlayerId);
    if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Ledger=Armed->ExportItemLedger();
    if(Ledger.Items.ContainsByPredicate([&](const FTUItemInstance& I){return I.InstanceId==ItemId;})) return false;
    FTUItemInstance Item; Item.InstanceId=ItemId; Item.OwnerId=P->PlayerId; Item.DefinitionId=DefinitionId; Item.FoundInRaidId=RaidState->RaidId;
    Ledger.Items.Add(Item); ++Ledger.Revision;
    if(UTUHideoutLifecycleSubsystem* Life=GetLifecycle())
    {
        FTUItemLedger Acquired; Acquired.Items.Add(Item);
        if(!Life->RecordRaidAcquisition(RaidState->RaidId,P->PlayerId,Acquired)) return false;
    }
    // Credit must be durable before consuming the world source. Failure leaves its stable event ID retryable.
    if(!RecordUniqueTaskEvent(Pawn,ItemId,ETUTaskCondition::Recover,DefinitionId)) return false;
    if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) if(!Armed->ImportItemLedger(Ledger)) return false;
    Ledgers.Add(P->PlayerId,Ledger);
    return true;
}
bool ATU_GameMode::HandoverItem(APawn* Pawn,FName TaskId,const FGuid& ItemId)
{
    FTURaidParticipantState* P=MutableParticipant(Pawn);
    if(!HasAuthority() || !P || bTrainingRaid || P->Outcome!=ETURaidPlayerOutcome::Active || P->bSavePending) return false;
    FTUTaskProgress* T=P->Tasks.FindByPredicate([&](const FTUTaskProgress& V){return V.Definition.TaskId==TaskId;});
    if(!T) return false;
    FTUItemLedger Ledger=Ledgers.FindChecked(P->PlayerId);
    if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Ledger=Armed->ExportItemLedger();
    FTUTaskProgress Updated=*T;
    if(!FTUTaskRules::Handover(Updated,Ledger,ItemId)) return false;
    if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) if(!Armed->ImportItemLedger(Ledger)) return false;
    *T=Updated; Ledgers.Add(P->PlayerId,Ledger); SyncPlayerState(Pawn); return true;
}
bool ATU_GameMode::BeginExtraction(APawn* Pawn,ATU_ExtractionZone* Zone)
{
    FTURaidParticipantState* P=MutableParticipant(Pawn);
    if(!HasAuthority() || !P || !Zone || P->Outcome!=ETURaidPlayerOutcome::Active || P->bSavePending || !Zone->IsEligible(Pawn,this)) return false;
    if(const UTUHealthComponent* Health=Pawn->FindComponentByClass<UTUHealthComponent>()) if(Health->IsDead()) return false;
    if(P->bExtracting) return ExtractionZones.FindRef(P->PlayerId).Get()==Zone;
    P->bExtracting=true; P->ExtractId=Zone->ExtractId;
    P->ExtractionEndTime=ElapsedTime+FMath::Max(0.f,Zone->HoldSeconds); ExtractionZones.Add(P->PlayerId,Zone);
    SyncPlayerState(Pawn); return true;
}
void ATU_GameMode::CancelExtraction(APawn* Pawn)
{
    if(!HasAuthority()) return;
    if(FTURaidParticipantState* P=MutableParticipant(Pawn))
    {
        if(P->bSavePending) return;
        P->bExtracting=false; P->ExtractionEndTime=0.f; P->ExtractId=NAME_None; ExtractionZones.Remove(P->PlayerId); SyncPlayerState(Pawn);
    }
}
bool ATU_GameMode::ResolvePlayerOutcome(APawn* Pawn,ETURaidPlayerOutcome Outcome,FName ExtractId)
{
    FTURaidParticipantState* P=MutableParticipant(Pawn);
    if(!HasAuthority() || !P || (P->Outcome!=ETURaidPlayerOutcome::Active && P->Outcome!=ETURaidPlayerOutcome::DisconnectedPendingResolution) ||
        Outcome==ETURaidPlayerOutcome::Active) return false;
    if(P->bSavePending)
    {
        const FTURaidOutcome* Pending=PendingOutcomes.Find(P->PlayerId);
        if(!Pending || Pending->Outcome!=Outcome || Pending->ExtractId!=ExtractId) return false;
    }
    if(Outcome==ETURaidPlayerOutcome::DisconnectedPendingResolution)
    {
        if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Ledgers.Add(P->PlayerId,Armed->ExportItemLedger());
        CancelExtraction(Pawn); P->Outcome=Outcome; SyncPlayerState(Pawn); return true;
    }
    if(!P->bSavePending && !FlushPendingTaskEvents(P->PlayerId)) return false;
    ATU_ExtractionZone* Zone=ExtractionZones.FindRef(P->PlayerId).Get();
    if(Outcome==ETURaidPlayerOutcome::Extracted && !P->bSavePending)
    {
        if(P->Outcome!=ETURaidPlayerOutcome::Active || !Zone || !P->bExtracting || P->ExtractionEndTime>ElapsedTime ||
            ExtractId!=Zone->ExtractId || !Zone->IsEligible(Pawn,this)) return false;
        if(const UTUHealthComponent* H=Pawn->FindComponentByClass<UTUHealthComponent>()) if(H->IsDead()) return false;
    }
    FTURaidOutcome Result; Result.RaidId=RaidState->RaidId; Result.PlayerId=P->PlayerId; Result.Outcome=Outcome;
    Result.ExtractId=ExtractId; Result.bTraining=bTrainingRaid;
    FTUItemLedger Ledger=Ledgers.FindChecked(P->PlayerId);
    if(!P->bSavePending)
        if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Ledger=Armed->ExportItemLedger();
    Ledgers.Add(P->PlayerId,Ledger);
    TArray<FTUTaskProgress> Tasks=BuildOutcomeTasks(P->Tasks,Result,Ledger);
    UTUHideoutLifecycleSubsystem* Life=GetLifecycle();
    // A durable-write retry reserves its exit capacity exactly once.
    if(Outcome==ETURaidPlayerOutcome::Extracted && Zone && !P->bSavePending) Zone->CommitCapacity();
    if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Armed->DisableCombatForOutcome();
    Pawn->SetCanBeDamaged(false);
    Pawn->SetActorEnableCollision(false);
    const bool Saved=Life?Life->CommitRaidOutcome(Result,Ledger,Tasks):bTrainingRaid;
    if(!Saved)
    {
        if(ATU_ArmedOperatorCharacter* Armed=Cast<ATU_ArmedOperatorCharacter>(Pawn)) Armed->DisableCombatForOutcome();
        P->bSavePending=true; P->bExtracting=false; PendingOutcomes.Add(P->PlayerId,Result);
        const int32 Attempt=SaveRetryCounts.FindOrAdd(P->PlayerId)++;
        NextSaveRetryTime.Add(P->PlayerId,ElapsedTime+FMath::Min(8.f,FMath::Pow(2.f,FMath::Min(Attempt,3))));
        SyncPlayerState(Pawn); return false;
    }
    P->Outcome=Outcome; P->bOutcomePersisted=Life!=nullptr; P->bSavePending=false; P->bExtracting=false; P->ExtractId=ExtractId; P->Tasks=Tasks;
    PendingOutcomes.Remove(P->PlayerId); ExtractionZones.Remove(P->PlayerId);
    NextSaveRetryTime.Remove(P->PlayerId); SaveRetryCounts.Remove(P->PlayerId);
    Pawn->SetActorEnableCollision(false);
    Pawn->SetActorHiddenInGame(true);
    SyncPlayerState(Pawn);
    if(APlayerController* PC=Cast<APlayerController>(Pawn->GetController()))
    { PC->UnPossess(); PC->ChangeState(NAME_Spectating); PC->ClientGotoState(NAME_Spectating); }
    SyncPlayerState(Pawn);
    ScheduleReturnIfResolved();
    return true;
}
void ATU_GameMode::ScheduleReturnIfResolved()
{
    if(!bReturnScheduled && GetLifecycle() && !LifecycleOverride && GetGameInstance() && RaidState &&
        !RaidState->Participants.ContainsByPredicate([](const FTURaidParticipantState& V){return V.Outcome==ETURaidPlayerOutcome::Active || V.Outcome==ETURaidPlayerOutcome::DisconnectedPendingResolution;}))
    {
        bReturnScheduled=true;
        FTimerHandle ReturnTimer;
        GetWorld()->GetTimerManager().SetTimer(ReturnTimer,FTimerDelegate::CreateWeakLambda(this,[this]()
        { if(UTUHideoutLifecycleSubsystem* Lifecycle=GetLifecycle()) Lifecycle->ReturnToHideout(false); }),5.f,false);
    }
}
void ATU_GameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(bRaidStarted && GetWorld())
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(APlayerController* PC=It->Get()) if(APawn* Pawn=PC->GetPawn()) RegisterParticipant(Pawn);
    AdvanceRaidTime(DeltaSeconds);
}
void ATU_GameMode::AdvanceRaidTime(float DeltaSeconds)
{
    if(!HasAuthority() || !bRaidStarted || DeltaSeconds<0.f || !FMath::IsFinite(DeltaSeconds)) return;
    ElapsedTime+=DeltaSeconds;
    RaidState->RaidElapsedTime=ElapsedTime;
    FlushPendingTaskEvents(FGuid());
    // Logout destroys pawns. Their escrow, tasks and terminal adjudication outlive that actor.
    for(const auto& Entry:PawnIds)
    {
        if(Entry.Key.IsValid()) continue;
        FTURaidParticipantState* Orphan=RaidState->Participants.FindByPredicate([&](const FTURaidParticipantState& V){return V.PlayerId==Entry.Value;});
        if(!Orphan || (Orphan->Outcome!=ETURaidPlayerOutcome::Active && Orphan->Outcome!=ETURaidPlayerOutcome::DisconnectedPendingResolution)) continue;
        Orphan->bExtracting=false;
        Orphan->Outcome=ETURaidPlayerOutcome::DisconnectedPendingResolution;
        if(ElapsedTime<RaidDurationSeconds && !Orphan->bSavePending) continue;
        if(!Orphan->bSavePending && !FlushPendingTaskEvents(Orphan->PlayerId)) continue;
        if(Orphan->bSavePending && ElapsedTime<NextSaveRetryTime.FindRef(Orphan->PlayerId)) continue;
        FTURaidOutcome Result;
        if(const FTURaidOutcome* Pending=PendingOutcomes.Find(Orphan->PlayerId)) Result=*Pending;
        else { Result.RaidId=RaidState->RaidId; Result.PlayerId=Orphan->PlayerId; Result.Outcome=ETURaidPlayerOutcome::TimedOut; Result.bTraining=bTrainingRaid; }
        TArray<FTUTaskProgress> Tasks=BuildOutcomeTasks(Orphan->Tasks,Result,Ledgers.FindChecked(Orphan->PlayerId));
        UTUHideoutLifecycleSubsystem* Life=GetLifecycle();
        const bool Saved=Life?Life->CommitRaidOutcome(Result,Ledgers.FindChecked(Orphan->PlayerId),Tasks):bTrainingRaid;
        Orphan->bSavePending=!Saved;
        if(Saved)
        {
            Orphan->Outcome=Result.Outcome; Orphan->Tasks=Tasks; Orphan->ExtractId=Result.ExtractId; Orphan->bOutcomePersisted=Life!=nullptr;
            PendingOutcomes.Remove(Orphan->PlayerId); NextSaveRetryTime.Remove(Orphan->PlayerId); SaveRetryCounts.Remove(Orphan->PlayerId);
            ExtractionZones.Remove(Orphan->PlayerId); RaidState->ForceNetUpdate(); ScheduleReturnIfResolved();
        }
        else
        {
            PendingOutcomes.Add(Orphan->PlayerId,Result);
            const int32 Attempt=SaveRetryCounts.FindOrAdd(Orphan->PlayerId)++;
            NextSaveRetryTime.Add(Orphan->PlayerId,ElapsedTime+FMath::Min(8.f,FMath::Pow(2.f,FMath::Min(Attempt,3))));
        }
    }
    TArray<TWeakObjectPtr<APawn>> Pawns; PawnIds.GenerateKeyArray(Pawns);
    for(const TWeakObjectPtr<APawn>& Weak:Pawns)
    {
        APawn* Pawn=Weak.Get(); FTURaidParticipantState* P=MutableParticipant(Pawn);
        if(!Pawn || !P) continue;
        if(P->bSavePending)
        {
            if(ElapsedTime<NextSaveRetryTime.FindRef(P->PlayerId)) continue;
            const FTURaidOutcome Pending=PendingOutcomes.FindChecked(P->PlayerId);
            ResolvePlayerOutcome(Pawn,Pending.Outcome,Pending.ExtractId); continue;
        }
        if(P->Outcome!=ETURaidPlayerOutcome::Active && P->Outcome!=ETURaidPlayerOutcome::DisconnectedPendingResolution) continue;
        if(const UTUHealthComponent* H=Pawn->FindComponentByClass<UTUHealthComponent>())
            if(H->IsDead()) { ResolvePlayerOutcome(Pawn,ETURaidPlayerOutcome::Dead); continue; }
        if(ElapsedTime>=RaidDurationSeconds) { ResolvePlayerOutcome(Pawn,ETURaidPlayerOutcome::TimedOut); continue; }
        if(P->bExtracting)
        {
            ATU_ExtractionZone* Zone=ExtractionZones.FindRef(P->PlayerId).Get();
            if(!Zone || !Zone->IsEligible(Pawn,this)) { CancelExtraction(Pawn); continue; }
            if(ElapsedTime>=P->ExtractionEndTime) ResolvePlayerOutcome(Pawn,ETURaidPlayerOutcome::Extracted,Zone->ExtractId);
        }
    }
}
void ATU_GameMode::Logout(AController* Exiting)
{
    if(Exiting && Exiting->GetPawn()) ResolvePlayerOutcome(Exiting->GetPawn(),ETURaidPlayerOutcome::DisconnectedPendingResolution);
    Super::Logout(Exiting);
}
