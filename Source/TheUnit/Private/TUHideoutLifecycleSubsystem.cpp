#include "TUHideoutLifecycleSubsystem.h"

#include "TUHideoutSaveGame.h"
#include "TUPersistence.h"
#include "TUTaskRules.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TUHideoutProgressionComponent.h"
#include "TUMeleeLoadoutComponent.h"
#include "TUMissionPackageData.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutComponent.h"
#include "TU_ModularOperatorCharacter.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    static constexpr ETUEquipmentSlot AllGearSlots[] = {
        ETUEquipmentSlot::Headwear,
        ETUEquipmentSlot::Headset,
        ETUEquipmentSlot::Eyewear,
        ETUEquipmentSlot::Facewear,
        ETUEquipmentSlot::NVG,
        ETUEquipmentSlot::TorsoArmor,
        ETUEquipmentSlot::ChestRig,
        ETUEquipmentSlot::Backpack,
        ETUEquipmentSlot::Belt,
        ETUEquipmentSlot::LeftHip,
        ETUEquipmentSlot::RightHip,
        ETUEquipmentSlot::Gloves,
        ETUEquipmentSlot::KneePads,
        ETUEquipmentSlot::Footwear,
        ETUEquipmentSlot::Accessory
    };
}

void UTUHideoutLifecycleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FString TestSlot;
    if (FParse::Value(FCommandLine::Get(), TEXT("TUProfileSlot="), TestSlot) && TestSlot.StartsWith(TEXT("TU_Automation_"))) SaveSlotName = TestSlot;
    // Isolate capture before any profile read/recovery, including malformed invocations.
    FString HandlingCaptureId;
    if (FParse::Value(FCommandLine::Get(), TEXT("TUHandlingCapture="), HandlingCaptureId)
        && !SaveSlotName.StartsWith(TEXT("TU_Automation_")))
    {
        FString SafeId;
        for (TCHAR C : HandlingCaptureId.Left(64)) if (FChar::IsAlnum(C) || C == TCHAR('_')) SafeId.AppendChar(C);
        SaveSlotName = TEXT("TU_Automation_Handling_") + (SafeId.IsEmpty() ? TEXT("Capture") : SafeId);
    }
    if (LoadProfile()) RecoverUnresolvedDeployments();
}


bool UTUHideoutLifecycleSubsystem::LoadProfile()
{
    // Prepared journal is not a committed result. Accept primary or prior backup only.
    Profile = nullptr;
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex)) Profile = Cast<UTUHideoutSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
    if (!Profile && UGameplayStatics::DoesSaveGameExist(SaveSlotName + TEXT("_backup"), SaveUserIndex)) Profile = Cast<UTUHideoutSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName + TEXT("_backup"), SaveUserIndex));
    const bool bExisting = UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex) || UGameplayStatics::DoesSaveGameExist(SaveSlotName + TEXT("_backup"), SaveUserIndex);
    if (!Profile && bExisting) { LastPersistenceError = TEXT("Primary and backup unreadable; refusing replacement"); return false; }
    if (!Profile) Profile = NewObject<UTUHideoutSaveGame>(this);
    if (Profile->SaveVersion > 3) { Profile = nullptr; LastPersistenceError = TEXT("Unsupported future schema"); return false; }
    if (!Profile->LocalPlayerId.IsValid()) Profile->LocalPlayerId = FGuid::NewGuid();
    if (Profile->SaveVersion < 3)
    {
        Profile->SaveVersion = 3;
        Profile->bMissionInProgress = false;
        Profile->ActiveMissionId = NAME_None;
        return SaveProfile();
    }
    return true;
}
bool UTUHideoutLifecycleSubsystem::WriteSlot(UTUHideoutSaveGame* Save, const FString& Slot)
{
    ++WriteNumber;
    if (FailWriteNumber > 0 && WriteNumber == FailWriteNumber) return false;
    return UGameplayStatics::SaveGameToSlot(Save, Slot, SaveUserIndex);
}
bool UTUHideoutLifecycleSubsystem::RecoverUnresolvedDeployments()
{
    bRecoveryBlocked = true;
    if (!Profile) return false;
    const TArray<FTUDeploymentRecord> Pending = Profile->Deployments;
    for (const auto& D : Pending)
    {
        if (D.bResolved) continue;
        FTURaidOutcome Abandoned; Abandoned.RaidId = D.RaidId; Abandoned.PlayerId = D.PlayerId;
        Abandoned.Outcome = ETURaidPlayerOutcome::Abandoned; Abandoned.bTraining = D.bTraining;
        TArray<FTUTaskProgress> Tasks;
        for (auto T : Profile->Tasks) if (T.PlayerId == D.PlayerId) { FTUTaskRules::Resolve(T, Abandoned); Tasks.Add(T); }
        if (!CommitRaidOutcome(Abandoned, FTUItemLedger(), Tasks)) return false;
    }
    bRecoveryBlocked = false;
    return true;
}
bool UTUHideoutLifecycleSubsystem::PersistCandidate(UTUHideoutSaveGame* Candidate)
{
    if (!Candidate || (GetWorld() && GetWorld()->GetNetMode() == NM_Client)) return false;
    WriteNumber = 0; LastPersistenceError.Reset();
    Candidate->CommitSequence = Profile ? Profile->CommitSequence + 1 : 1;
    if (!WriteSlot(Candidate, SaveSlotName + TEXT("_journal")) ||
        !WriteSlot(Profile ? Profile.Get() : Candidate, SaveSlotName + TEXT("_backup")) ||
        !WriteSlot(Candidate, SaveSlotName))
    {
        LastPersistenceError = FString::Printf(TEXT("Write %d failed; prior profile retained; retry available"), WriteNumber);
        return false;
    }
    Profile = Candidate;
    UGameplayStatics::DeleteGameInSlot(SaveSlotName + TEXT("_journal"), SaveUserIndex);
    return true;
}
bool UTUHideoutLifecycleSubsystem::SaveProfile()
{
    if (!Profile && !LoadProfile()) return false;
    return PersistCandidate(DuplicateObject<UTUHideoutSaveGame>(Profile, this));
}
void UTUHideoutLifecycleSubsystem::ConfigureTestSlot(const FString& UniqueSlot)
{
    checkf(UniqueSlot.StartsWith(TEXT("TU_Automation_")), TEXT("Tests require a unique TU_Automation_ slot"));
    SaveSlotName = UniqueSlot; Profile = nullptr; FailWriteNumber = 0;
}
FGuid UTUHideoutLifecycleSubsystem::GetLocalPlayerId() const { return Profile ? Profile->LocalPlayerId : FGuid(); }
FGuid UTUHideoutLifecycleSubsystem::GetOrCreatePlayerId(const FString& StableKey)
{
    if (!Profile || StableKey.IsEmpty()) return FGuid();
    if (const auto* ID = Profile->KnownPlayers.Find(StableKey)) return *ID;
    auto* Candidate = DuplicateObject<UTUHideoutSaveGame>(Profile, this);
    const FGuid NewId = FGuid::NewGuid(); Candidate->KnownPlayers.Add(StableKey, NewId);
    return PersistCandidate(Candidate) ? NewId : FGuid();
}
FGuid UTUHideoutLifecycleSubsystem::GetActiveRaidId() const
{
    return GetActiveRaidIdForPlayer(GetLocalPlayerId());
}
FGuid UTUHideoutLifecycleSubsystem::GetActiveRaidIdForPlayer(const FGuid& PlayerId) const
{
    if (Profile) for (const auto& D : Profile->Deployments) if (!D.bResolved && D.PlayerId == PlayerId) return D.RaidId;
    return FGuid();
}
FTUItemLedger UTUHideoutLifecycleSubsystem::GetPlayerStash(const FGuid& PlayerId) const
{
    FTUItemLedger Result;
    if (Profile)
    {
        Result = TUPersistence::OwnedBy(Profile->Stash, PlayerId);
        if (const auto* Loose = Profile->LooseByPlayer.Find(PlayerId)) Result.LooseCartridges = Loose->LooseCartridges;
    }
    return Result;
}
bool UTUHideoutLifecycleSubsystem::CaptureInitialKit(const FTUItemLedger& L) { return CaptureInitialKitForPlayer(GetLocalPlayerId(), L); }
bool UTUHideoutLifecycleSubsystem::CaptureInitialKitForPlayer(const FGuid& PlayerId, const FTUItemLedger& Ledger)
{
    if (!Profile || !TUPersistence::ValidateLedger(Ledger, PlayerId)) return false;
    if (Profile->InitializedPlayers.Contains(PlayerId)) return true;
    auto* Candidate = DuplicateObject<UTUHideoutSaveGame>(Profile, this);
    TSet<FGuid> Existing;
    auto AddIds = [&Existing](const FTUItemLedger& L) { for (const auto& W : L.Weapons) Existing.Add(W.InstanceId); for (const auto& M : L.Magazines) Existing.Add(M.InstanceId); for (const auto& I : L.Items) Existing.Add(I.InstanceId); };
    AddIds(Candidate->Stash); for (const auto& D : Candidate->Deployments) AddIds(D.Escrow);
    for (const auto& W : Ledger.Weapons) if (Existing.Contains(W.InstanceId)) return false;
    for (const auto& M : Ledger.Magazines) if (Existing.Contains(M.InstanceId)) return false;
    for (const auto& I : Ledger.Items) if (Existing.Contains(I.InstanceId)) return false;
    FTUItemLedger ItemsOnly = Ledger; ItemsOnly.LooseCartridges.Reset();
    TUPersistence::Append(Candidate->Stash, ItemsOnly);
    Candidate->LooseByPlayer.FindOrAdd(PlayerId).LooseCartridges.Append(Ledger.LooseCartridges);
    Candidate->InitializedPlayers.Add(PlayerId);
    if (PlayerId == Candidate->LocalPlayerId) Candidate->bInitialKitCaptured = true;
    return PersistCandidate(Candidate);
}

bool UTUHideoutLifecycleSubsystem::BeginDeployment(const FGuid& RaidId, const FGuid& PlayerId, const FTUItemLedger& Ledger, bool bTraining)
{
    FTUDeploymentRecord D; D.RaidId=RaidId; D.PlayerId=PlayerId; D.Escrow=Ledger; D.bTraining=bTraining;
    return BeginDeploymentBatch({D});
}
bool UTUHideoutLifecycleSubsystem::BeginDeploymentBatch(const TArray<FTUDeploymentRecord>& Deployments)
{
    if (bRecoveryBlocked || !Profile || Deployments.IsEmpty()) return false;
    auto* Candidate=DuplicateObject<UTUHideoutSaveGame>(Profile,this);
    TSet<FGuid> Players; bool Changed=false;
    for (const auto& D : Deployments)
    {
        if (!D.RaidId.IsValid() || D.RaidId!=Deployments[0].RaidId || D.bTraining!=Deployments[0].bTraining || D.bResolved || Players.Contains(D.PlayerId) || !TUPersistence::ValidateLedger(D.Escrow,D.PlayerId)) return false;
        Players.Add(D.PlayerId);
        bool Existing=false;
        for (const auto& Prior : Candidate->Deployments)
        {
            if (Prior.PlayerId!=D.PlayerId) continue;
            if (Prior.RaidId==D.RaidId)
            {
                if (Prior.bResolved || Prior.bTraining!=D.bTraining || !TUPersistence::ContainsExact(Prior.Escrow,D.Escrow) || !TUPersistence::ContainsExact(D.Escrow,Prior.Escrow)) return false;
                Existing=true;
            }
            else if (!Prior.bResolved) return false;
        }
        if (Existing) continue;
        if (!D.bTraining)
        {
            FTUItemLedger Available=TUPersistence::OwnedBy(Candidate->Stash,D.PlayerId);
            Available.LooseCartridges=Candidate->LooseByPlayer.FindOrAdd(D.PlayerId).LooseCartridges;
            if (!Candidate->InitializedPlayers.Contains(D.PlayerId) || !TUPersistence::ContainsExact(Available,D.Escrow)) return false;
            TUPersistence::Remove(Candidate->Stash,D.Escrow);
            auto& Loose=Candidate->LooseByPlayer.FindOrAdd(D.PlayerId).LooseCartridges;
            for (const FName Ammo : D.Escrow.LooseCartridges) { const int32 I=Loose.Find(Ammo); if (I!=INDEX_NONE) Loose.RemoveAt(I); }
        }
        Candidate->Deployments.Add(D); Changed=true;
        if (D.PlayerId==Candidate->LocalPlayerId) Candidate->bMissionInProgress=true;
    }
    return !Changed || PersistCandidate(Candidate);
}
bool UTUHideoutLifecycleSubsystem::CheckpointTasks(const FGuid& RaidId,const TArray<FTUTaskProgress>& Candidates)
{
    if (!Profile || !RaidId.IsValid() || bRecoveryBlocked) return false;
    auto* Candidate=DuplicateObject<UTUHideoutSaveGame>(Profile,this);
    TSet<FString> Keys; bool Changed=false;
    for (const auto& Task : Candidates)
    {
        const auto* D=Candidate->Deployments.FindByPredicate([&](const auto& V) { return V.RaidId==RaidId && V.PlayerId==Task.PlayerId && !V.bResolved; });
        if (!D) return false;
        if (D->bTraining || Task.Definition.Policy!=ETUTaskPolicy::Cumulative) continue;
        const FString Key=Task.PlayerId.ToString()+TEXT(":")+Task.Definition.TaskId.ToString();
        if (Keys.Contains(Key) || Task.Definition.TaskId.IsNone() || Task.CommittedCount<0 || Task.PendingCount!=0 || !Task.ConsumedItemIds.IsEmpty()) return false;
        Keys.Add(Key);
        TSet<FGuid> EventIds;
        for (const auto& Event : Task.ProcessedEventIds) { if (!Event.IsValid() || EventIds.Contains(Event)) return false; EventIds.Add(Event); }
        auto* Prior=Candidate->Tasks.FindByPredicate([&](const auto& T) { return T.PlayerId==Task.PlayerId && T.Definition.TaskId==Task.Definition.TaskId; });
        if (Prior)
        {
            if (!FTUTaskDefinition::StaticStruct()->CompareScriptStruct(&Prior->Definition,&Task.Definition,0) || Task.CommittedCount<Prior->CommittedCount || (Prior->bCompleted && !Task.bCompleted)) return false;
            for (const auto& Event : Prior->ProcessedEventIds) if (!EventIds.Contains(Event)) return false;
            if (Task.CommittedSteps.Num()<Prior->CommittedSteps.Num()) return false;
            for (int32 I=0;I<Prior->CommittedSteps.Num();++I) if (Task.CommittedSteps[I]<Prior->CommittedSteps[I]) return false;
            if (FTUTaskProgress::StaticStruct()->CompareScriptStruct(Prior,&Task,0)) continue;
            *Prior=Task;
        }
        else Candidate->Tasks.Add(Task);
        Changed=true;
    }
    return !Changed || PersistCandidate(Candidate);
}
bool UTUHideoutLifecycleSubsystem::RecordRaidAcquisition(const FGuid& RaidId, const FGuid& PlayerId, const FTUItemLedger& Acquired)
{
    if (!Profile || !TUPersistence::ValidateLedger(Acquired, PlayerId) || !Acquired.Weapons.IsEmpty() || !Acquired.LooseCartridges.IsEmpty()) return false;
    const auto* Deployment = Profile->Deployments.FindByPredicate([&](const auto& D) { return D.RaidId == RaidId && D.PlayerId == PlayerId && !D.bResolved; });
    if (!Deployment) return false;
    if (Deployment->bTraining) return true;
    auto* Candidate = DuplicateObject<UTUHideoutSaveGame>(Profile, this);
    TSet<FGuid> IDs;
    for (const auto& M : Acquired.Magazines) IDs.Add(M.InstanceId);
    for (const auto& I : Acquired.Items) { if (I.FoundInRaidId != RaidId) return false; IDs.Add(I.InstanceId); }
    for (const auto& W : Candidate->Stash.Weapons) if (IDs.Contains(W.InstanceId)) return false;
    for (const auto& M : Candidate->Stash.Magazines) if (IDs.Contains(M.InstanceId)) return false;
    for (const auto& I : Candidate->Stash.Items) if (IDs.Contains(I.InstanceId)) return false;
    // Revoke old allowance first, including same-owner drop/pickup; never duplicate ammo budget.
    for (auto& D : Candidate->Deployments)
    {
        if (D.bResolved) continue;
        for (const auto& M : D.Escrow.Magazines) if (IDs.Contains(M.InstanceId))
        {
            if (D.RaidId != RaidId || D.bTraining) return false;
            const auto* Transfer = Acquired.Magazines.FindByPredicate([&](const auto& A) { return A.InstanceId == M.InstanceId; });
            if (!Transfer || Transfer->DefinitionId != M.DefinitionId || Transfer->Capacity != M.Capacity) return false;
            TArray<FName> Budget = M.Cartridges;
            for (const FName Ammo : Transfer->Cartridges) { const int32 Index = Budget.Find(Ammo); if (Index == INDEX_NONE) return false; Budget.RemoveAt(Index); }
            // Keep ammunition that left this magazine by cycling in the donor allowance.
            // Transfer reduces the donor total by actual acquired cartridges, not initial magazine count.
            D.Escrow.LooseCartridges.Append(Budget);
        }
        for (auto& W : D.Escrow.Weapons) if (IDs.Contains(W.InsertedMagazineId)) W.InsertedMagazineId.Invalidate();
        D.Escrow.Magazines.RemoveAll([&](const auto& M) { return IDs.Contains(M.InstanceId); });
        D.Escrow.Items.RemoveAll([&](const auto& I) { return IDs.Contains(I.InstanceId); });
    }
    auto* Target = Candidate->Deployments.FindByPredicate([&](const auto& D) { return D.RaidId == RaidId && D.PlayerId == PlayerId && !D.bResolved; });
    TUPersistence::Append(Target->Escrow, Acquired);
    return PersistCandidate(Candidate);
}
bool UTUHideoutLifecycleSubsystem::CommitTaskHandover(const FGuid& PlayerId, FName TaskId, const FGuid& ItemId)
{
    if (!Profile) return false;
    for (const auto& D : Profile->Deployments) if (D.PlayerId == PlayerId && !D.bResolved) return false;
    auto* Candidate = DuplicateObject<UTUHideoutSaveGame>(Profile, this);
    auto* Task = Candidate->Tasks.FindByPredicate([&](const auto& T) { return T.PlayerId == PlayerId && T.Definition.TaskId == TaskId; });
    if (!Task) return false;
    if (Task->ConsumedItemIds.Contains(ItemId)) return true;
    if (!FTUTaskRules::Handover(*Task, Candidate->Stash, ItemId)) return false;
    Candidate->Stash.Items.RemoveAll([&](const auto& I) { return I.InstanceId == ItemId; });
    return PersistCandidate(Candidate);
}
bool UTUHideoutLifecycleSubsystem::CommitRaidOutcome(const FTURaidOutcome& Outcome, const FTUItemLedger& Ledger, const TArray<FTUTaskProgress>& Tasks)
{
    if (!Profile || Outcome.Revision < 1 || Outcome.Outcome == ETURaidPlayerOutcome::Active || Outcome.Outcome == ETURaidPlayerOutcome::DisconnectedPendingResolution) return false;
    for (const auto& Previous : Profile->Outcomes)
        if (Previous.RaidId == Outcome.RaidId && Previous.PlayerId == Outcome.PlayerId)
            return Previous.Revision == Outcome.Revision && Previous.Outcome == Outcome.Outcome && Previous.ExtractId == Outcome.ExtractId && Previous.bTraining == Outcome.bTraining;
    const auto* Deployment = Profile->Deployments.FindByPredicate([&](const auto& D) { return D.RaidId == Outcome.RaidId && D.PlayerId == Outcome.PlayerId && !D.bResolved; });
    if (!Deployment || Deployment->bTraining != Outcome.bTraining ||
        (Outcome.bTraining ? !TUPersistence::ValidateLedger(Ledger,Outcome.PlayerId) : !TUPersistence::ValidateReturn(Ledger,Deployment->Escrow,Outcome.PlayerId,Outcome.RaidId))) return false;
    for (const auto& Task : Tasks) if (Task.PlayerId != Outcome.PlayerId || Task.CommittedCount < 0 || Task.PendingCount < 0) return false;
    auto* Candidate = DuplicateObject<UTUHideoutSaveGame>(Profile, this);
    for (auto& D : Candidate->Deployments) if (D.RaidId == Outcome.RaidId && D.PlayerId == Outcome.PlayerId) { D.bResolved = true; D.Escrow = FTUItemLedger(); }
    Candidate->Outcomes.Add(Outcome);
    if (!Outcome.bTraining)
    {
        if (Outcome.Outcome == ETURaidPlayerOutcome::Extracted)
        {
            FTUItemLedger Surviving = Ledger;
            Surviving.Weapons.RemoveAll([](const auto& W) { return W.Location == ETUItemLocation::Ground || W.Location == ETUItemLocation::Consumed; });
            Surviving.Magazines.RemoveAll([](const auto& M) { return M.Location == ETUItemLocation::Ground || M.Location == ETUItemLocation::Consumed; });
            Surviving.Items.RemoveAll([](const auto& I) { return I.Location == ETUItemLocation::Ground || I.Location == ETUItemLocation::Consumed; });
            for (auto& I : Surviving.Items) { I.bExtracted = true; I.ExtractedAtId = Outcome.ExtractId; }
            FTUItemLedger ItemsOnly = Surviving; ItemsOnly.LooseCartridges.Reset();
            TUPersistence::Append(Candidate->Stash, ItemsOnly);
            Candidate->LooseByPlayer.FindOrAdd(Outcome.PlayerId).LooseCartridges.Append(Surviving.LooseCartridges);
            ++Candidate->CompletedOperations;
        }

        for (const auto& Task : Tasks)
        {
            auto* Prior=Candidate->Tasks.FindByPredicate([&](const auto& T) { return T.PlayerId==Task.PlayerId && T.Definition.TaskId==Task.Definition.TaskId; });
            if (Prior && Prior->Definition.Policy==ETUTaskPolicy::Cumulative)
            {
                if (Task.CommittedCount<Prior->CommittedCount || (Prior->bCompleted && !Task.bCompleted)) return false;
                for (const auto& Event : Prior->ProcessedEventIds) if (!Task.ProcessedEventIds.Contains(Event)) return false;
            }
            if (Prior) *Prior=Task; else Candidate->Tasks.Add(Task);
        }
    }
    if (Outcome.PlayerId == Candidate->LocalPlayerId) { Candidate->bMissionInProgress = false; Candidate->ActiveMissionId = NAME_None; }
    return PersistCandidate(Candidate);
}

void UTUHideoutLifecycleSubsystem::ApplyHideoutState(UTUHideoutProgressionComponent* Progression) const
{
    if (!Progression || !Profile || Profile->HideoutModules.IsEmpty())
    {
        return;
    }

    for (const FTUHideoutModuleState& State : Profile->HideoutModules)
    {
        Progression->SetModuleLevel(State.Type, State.Level);
    }
}

void UTUHideoutLifecycleSubsystem::CaptureHideoutState(const UTUHideoutProgressionComponent* Progression)
{
    if (!Progression || !Profile)
    {
        return;
    }
    Profile->HideoutModules = Progression->GetModules();
}

void UTUHideoutLifecycleSubsystem::ApplyOperatorLoadout(ATU_ArmedOperatorCharacter* Operator) const
{
    if (!Operator || !Profile)
    {
        return;
    }
    const APlayerController* PC = Cast<APlayerController>(Operator->GetController());
    if (!PC || !PC->IsLocalController()) return; // Guest hydration is keyed by authority raid registration.
    if (bRecoveryBlocked) { Operator->ImportItemLedger(FTUItemLedger()); return; }

    if (!Profile->PrimaryId.IsNone())
    {
        Operator->SelectPrimaryById(Profile->PrimaryId);
    }
    if (!Profile->SecondaryId.IsNone())
    {
        Operator->SelectSecondaryById(Profile->SecondaryId);
    }
    if (!Profile->EquipmentId.IsNone())
    {
        Operator->SelectEquipmentById(Profile->EquipmentId);
    }
    if (!Profile->MeleeId.IsNone())
    {
        Operator->SelectMeleeById(Profile->MeleeId);
    }

    if (!Operator->HasHydratedInventory() && Profile->bInitialKitCaptured)
    {
        FTUItemLedger Hydrated = GetPlayerStash(GetLocalPlayerId());
        for (const auto& D : Profile->Deployments) if (!D.bResolved && D.PlayerId == GetLocalPlayerId()) { Hydrated = D.Escrow; break; }
        Operator->ImportItemLedger(Hydrated);
    }

    ATU_ModularOperatorCharacter* Modular = Cast<ATU_ModularOperatorCharacter>(Operator);
    if (!Modular || Profile->GearBySlot.IsEmpty())
    {
        return;
    }

    if (UTUOperatorEquipmentComponent* Equipment = Modular->GetOperatorEquipment())
    {
        Equipment->ClearLoadout();
    }

    for (const ETUEquipmentSlot Slot : AllGearSlots)
    {
        if (const FName* SavedItemId = Profile->GearBySlot.Find(Slot))
        {
            if (!SavedItemId->IsNone())
            {
                Modular->EquipGearById(*SavedItemId);
            }
        }
    }
}

void UTUHideoutLifecycleSubsystem::CaptureOperatorLoadout(const ATU_ArmedOperatorCharacter* Operator)
{
    if (!Operator || !Profile)
    {
        return;
    }
    const APlayerController* PC = Cast<APlayerController>(Operator->GetController());
    if (!PC || !PC->IsLocalController()) return;

    if (const UTUOperatorLoadoutComponent* Loadout = Operator->GetOperatorLoadout())
    {
        Profile->PrimaryId = Loadout->GetSelectedPrimaryId();
        Profile->SecondaryId = Loadout->GetSelectedSecondaryId();
        Profile->EquipmentId = Loadout->GetSelectedEquipmentId();
    }
    Profile->MeleeId = Operator->GetSelectedMeleeId();

    if (const ATU_ModularOperatorCharacter* Modular = Cast<ATU_ModularOperatorCharacter>(Operator))
    {
        Profile->GearBySlot.Reset();
        for (const ETUEquipmentSlot Slot : AllGearSlots)
        {
            Profile->GearBySlot.Add(Slot, Modular->GetEquippedGearId(Slot));
        }
    }
}



bool UTUHideoutLifecycleSubsystem::DeployToMission(ATU_ArmedOperatorCharacter* Operator,const UTUMissionPackageData* MissionPackage)
{
    const APlayerController* Initiator=Operator?Cast<APlayerController>(Operator->GetController()):nullptr;
    if (!Operator || !Operator->HasAuthority() || !Initiator || !Initiator->IsLocalController() || !GetWorld() || !MissionPackage || MissionPackage->DestinationMap.IsNone() || !MissionPackage->Mission.bDeploymentAuthorized) return false;
    if ((!Profile && !LoadProfile()) || bRecoveryBlocked) return false;
    for (const auto& D : Profile->Deployments) if (!D.bResolved) { LastPersistenceError=TEXT("Resolve all pending deployments before shared travel"); return false; }
    CaptureOperatorLoadout(Operator);
    const FGuid RaidId=FGuid::NewGuid(); TArray<FTUDeploymentRecord> Deployments;
    for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
    {
        APlayerController* PC=It->Get();
        ATU_ArmedOperatorCharacter* Pawn=PC?Cast<ATU_ArmedOperatorCharacter>(PC->GetPawn()):nullptr;
        ATU_PlayerState* PS=PC?PC->GetPlayerState<ATU_PlayerState>():nullptr;
        if (!PC || !Pawn || !PS) { LastPersistenceError=TEXT("Every connected player must have a ready operator before shared deployment"); return false; }
        FGuid PlayerId=PC->IsLocalController()?GetLocalPlayerId():PS->PersistentPlayerId;
        if (!PlayerId.IsValid())
        {
            const FString Key=PS->GetUniqueId().IsValid()?PS->GetUniqueId().ToString():PS->GetPlayerName();
            PlayerId=GetOrCreatePlayerId(Key);
        }
        if (!PlayerId.IsValid()) return false;
        // Deployment escrow is the operator's prepared carried ledger, never the whole persistent stash.
        // The stash remains the authority that proves those exact instances are owned and available.
        FTUItemLedger Ledger=Pawn->ExportItemLedger();
        for (auto& W : Ledger.Weapons) W.OwnerId=PlayerId;
        for (auto& M : Ledger.Magazines) M.OwnerId=PlayerId;
        for (auto& I : Ledger.Items) I.OwnerId=PlayerId;
        if (!MissionPackage->bTrainingOnly)
        {
            if (!Profile->InitializedPlayers.Contains(PlayerId) && !CaptureInitialKitForPlayer(PlayerId,Ledger)) return false;
            FTUItemLedger Available=GetPlayerStash(PlayerId);
            if (!TUPersistence::ContainsExact(Available,Ledger))
            {
                LastPersistenceError=TEXT("Prepared kit is not fully available in the persistent stash");
                return false;
            }
        }
        if (!Pawn->ImportItemLedger(Ledger)) return false;
        PS->PersistentPlayerId=PlayerId; PS->ForceNetUpdate();
        FTUDeploymentRecord D; D.RaidId=RaidId; D.PlayerId=PlayerId; D.Escrow=Ledger; D.bTraining=MissionPackage->bTrainingOnly; Deployments.Add(D);
    }
    if (Deployments.IsEmpty()) return false;
    auto* Before=DuplicateObject<UTUHideoutSaveGame>(Profile,this);
    Profile->ActiveMissionId=MissionPackage->Mission.MissionId;
    if (!BeginDeploymentBatch(Deployments)) { Profile->ActiveMissionId=Before->ActiveMissionId; return false; }
    if (GetWorld()->GetNetMode()==NM_Standalone) { UGameplayStatics::OpenLevel(Operator,MissionPackage->DestinationMap); return true; }
    if (GetWorld()->ServerTravel(MissionPackage->DestinationMap.ToString(),false)) return true;
    // Travel was never accepted. Restore stash ownership through the same durable commit path.
    const bool Restored=PersistCandidate(Before);
    LastPersistenceError=Restored?TEXT("Server travel rejected; deployment ownership restored"):TEXT("Server travel and rollback save failed; recovery required");
    bRecoveryBlocked=!Restored;
    return false;
}
bool UTUHideoutLifecycleSubsystem::ReturnToHideout(bool bOperationCompleted)
{
    if ((!Profile && !LoadProfile()) || bRecoveryBlocked || !GetGameInstance() || !GetWorld() || GetWorld()->GetNetMode()==NM_Client) return false;
    // No player can travel the shared session away from an unresolved teammate.
    for (const auto& D : Profile->Deployments) if (!D.bResolved) { LastPersistenceError=TEXT("Every participant must durably resolve before shared return"); return false; }
    const FName ReturnMap=Profile->HideoutMapName.IsNone()?FName(TEXT("CommandCenter")):Profile->HideoutMapName;
    if (GetWorld()->GetNetMode()==NM_Standalone) { UGameplayStatics::OpenLevel(GetGameInstance(),ReturnMap); return true; }
    return GetWorld()->ServerTravel(ReturnMap.ToString(),false);
}

bool UTUHideoutLifecycleSubsystem::IsMissionInProgress() const
{
    return Profile && Profile->bMissionInProgress;
}

FName UTUHideoutLifecycleSubsystem::GetActiveMissionId() const
{
    return Profile ? Profile->ActiveMissionId : NAME_None;
}

void UTUHideoutLifecycleSubsystem::SetHideoutMapName(FName MapName)
{
    if (!MapName.IsNone() && Profile)
    {
        Profile->HideoutMapName = MapName;
        SaveProfile();
    }
}
