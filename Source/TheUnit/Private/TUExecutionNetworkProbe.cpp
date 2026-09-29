#include "TUExecutionNetworkProbe.h"

#include "TU_ArmedOperatorCharacter.h"
#include "TU_WeaponBase.h"
#include "TU_GameMode.h"
#include "TU_GameState.h"
#include "TU_ExtractionZone.h"
#include "TU_DonetskDistrictGenerator.h"
#include "TU_DonetskArtema60Building.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "TUHealthComponent.h"
#include "TU_RaidCombatant.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace
{
    struct FNetworkRunVerdict
    {
        FString Role;
        bool bFinal = false;
        bool bPassed = false;
        bool bExitScheduled = false;
    };
    // Survives controller replacement, disconnect travel and map teardown in this process.
    TMap<FString, FNetworkRunVerdict> NetworkRunVerdicts;

    int32 NetworkProbeRounds(const FTUItemLedger& Ledger)
    {
        int32 Count = Ledger.LooseCartridges.Num();
        for (const auto& Weapon : Ledger.Weapons) Count += Weapon.ChamberAmmoId.IsNone() ? 0 : 1;
        for (const auto& Magazine : Ledger.Magazines) Count += Magazine.Cartridges.Num();
        return Count;
    }
}

UTUExecutionNetworkProbe::UTUExecutionNetworkProbe()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UTUExecutionNetworkProbe::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    bEnabled = FParse::Value(FCommandLine::Get(), TEXT("TUNetworkSmoke="), RunId) && !RunId.IsEmpty();
    for (TCHAR Character : RunId)
        bEnabled &= FChar::IsAlnum(Character) || Character == TCHAR('_') || Character == TCHAR('-');
    FString Slot;
    bEnabled &= FParse::Value(FCommandLine::Get(), TEXT("TUProfileSlot="), Slot) && Slot.StartsWith(TEXT("TU_Automation_"));
    ProfileName = TEXT("ordinary");
    FParse::Value(FCommandLine::Get(), TEXT("TUNetworkProfile="), ProfileName);
    bEnabled &= ProfileName == TEXT("ordinary") || ProfileName == TEXT("harsh");
#endif
    if (bEnabled)
    {
        FNetworkRunVerdict& Verdict = NetworkRunVerdicts.FindOrAdd(RunId);
        if (Verdict.Role.IsEmpty())
        {
            if (GetNetMode() == NM_Client) Verdict.Role = TEXT("guest");
            else if (GetNetMode() == NM_ListenServer) Verdict.Role = TEXT("host");
        }
        ProcessRole = Verdict.Role;
        if (Verdict.bFinal) bEnabled = false;
    }
    StartMap = GetWorld() ? GetWorld()->GetMapName() : FString();
    SetComponentTickEnabled(bEnabled);
}

bool UTUExecutionNetworkProbe::IsEnabledAuthority() const
{
    return bEnabled && GetOwner() && GetOwner()->HasAuthority() && GetNetMode() == NM_ListenServer;
}

bool UTUExecutionNetworkProbe::ApplyProfile()
{
    if (bProfileApplied) return true;
    UNetDriver* Driver = GetWorld() ? GetWorld()->GetNetDriver() : nullptr;
    if (!Driver) return false;
#if DO_ENABLE_NET_TEST
    FPacketSimulationSettings Settings;
    Settings.PktLag = ProfileName == TEXT("harsh") ? 150 : 50;
    Settings.PktLoss = ProfileName == TEXT("harsh") ? 5 : 1;
    Driver->SetPacketSimulationSettings(Settings);
    bProfileApplied = Driver->PacketSimulationSettings.PktLag == Settings.PktLag
        && Driver->PacketSimulationSettings.PktLoss == Settings.PktLoss
        && Driver->PacketSimulationSettings.PktIncomingLagMin == 0
        && Driver->PacketSimulationSettings.PktIncomingLagMax == 0
        && Driver->PacketSimulationSettings.PktIncomingLoss == 0;
    UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke %s outgoing lag=%dms loss=%d%% incoming=0; nominal RTT=%dms"),
        *ProfileName, Settings.PktLag, Settings.PktLoss, Settings.PktLag * 2);
#else
    Fail(TEXT("engine packet emulation unavailable in this build"));
#endif
    return bProfileApplied;
}

void UTUExecutionNetworkProbe::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bEnabled) return;
    if (const FNetworkRunVerdict* Verdict = NetworkRunVerdicts.Find(RunId); Verdict && Verdict->bFinal)
    {
        SetComponentTickEnabled(false);
        return;
    }
    Elapsed += DeltaTime;
    if (IsEnabledAuthority())
    {
        const APlayerController* LocalController = Cast<APlayerController>(GetOwner());
        if (LocalController && LocalController->IsLocalController())
        {
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
                if (APlayerController* Remote = It->Get(); Remote && !Remote->IsLocalController()
                    && Remote->FindComponentByClass<UTUExecutionNetworkProbe>())
                {
                    // Remote owned probe is now the sole host result/timeout writer.
                    SetComponentTickEnabled(false);
                    return;
                }
        }
    }
    if (bFinished) return;
    if (Elapsed > 120.f) { Fail(TEXT("120-second network gameplay deadline exceeded")); return; }
    const bool bTerminalEvidenceComplete = GetOwner()->HasAuthority() ? ServerStage >= 11 : bFinalOutcomesObserved;
    if (GetWorld()->GetMapName() != StartMap && !bTerminalEvidenceComplete)
    { Fail(TEXT("shared raid world changed before terminal outcomes were observed")); return; }
    if (!ApplyProfile()) return;
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!Controller) { Fail(TEXT("probe requires an owning PlayerController")); return; }
    if (!ObservedPawn) ObservedPawn = Cast<ATU_ArmedOperatorCharacter>(Controller->GetPawn());
    if (Elapsed - LastDiagnosticTime >= 5.f)
    {
        LastDiagnosticTime = Elapsed;
        const ATU_WeaponBase* Weapon = ObservedPawn ? ObservedPawn->GetPrimaryWeapon() : nullptr;
        UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke diagnostic authority=%d stage=%d expected=%d ack=%d pending=%d localAction=%d slot=%d phase=%d active=%d revision=%d"),
            GetOwner()->HasAuthority(), ServerStage, ExpectedCheckpoint, LastAcknowledged, PendingCheckpoint, LocalAction,
            ObservedPawn ? static_cast<int32>(ObservedPawn->GetActiveWeaponSlot()) : -1,
            Weapon ? static_cast<int32>(Weapon->GetActionState().Phase) : -1, Weapon && Weapon->GetActionState().bActive,
            Weapon ? Weapon->ExportItemLedger().Revision : -1);
        if (PendingCheckpoint >= 0 && !ExpectedSnapshot.IsEmpty())
        {
            const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TUNetworkSmoke"));
            IFileManager::Get().MakeDirectory(*Directory, true);
            const FString Prefix = FPaths::Combine(Directory, RunId + FString::Printf(TEXT(".checkpoint%d"), PendingCheckpoint));
            FFileHelper::SaveStringToFile(ExpectedSnapshot, *(Prefix + TEXT(".expected.txt")));
            FFileHelper::SaveStringToFile(PawnSnapshot(), *(Prefix + TEXT(".actual.txt")));
        }
    }
    if (GetNetMode() == NM_Client && Controller->IsLocalController()) LocalTick();
    else if (IsEnabledAuthority() && !Controller->IsLocalController()) ServerTick();
    // The host's local controller is deliberately passive; remote owned probe coordinates the pair.
}

FString UTUExecutionNetworkProbe::PawnSnapshot() const
{
    if (!ObservedPawn) return FString();
    const FString Geometry = CaptureGeometry();
    if (Geometry.IsEmpty()) return FString();
    FTUItemLedger Ledger = ObservedPawn->ExportItemLedger();
    for (auto& Action : Ledger.WeaponActions) Action.PhaseEndServerTime = 0.f;
    FString Text;
    FTUItemLedger::StaticStruct()->ExportText(Text, &Ledger, nullptr, nullptr, PPF_None, nullptr);
    return FString::Printf(TEXT("Slot=%d\n"), static_cast<int32>(ObservedPawn->GetActiveWeaponSlot())) + Text + TEXT("\nGeometry=") + Geometry;
}

FString UTUExecutionNetworkProbe::CaptureGeometry() const
{
    if (!GeometrySnapshot.IsEmpty()) return GeometrySnapshot;
    ATU_DonetskDistrictGenerator* District = nullptr;
    ATU_DonetskArtema60Building* Building = nullptr;
    for (TActorIterator<ATU_DonetskDistrictGenerator> It(GetWorld()); It; ++It) { District = *It; break; }
    for (TActorIterator<ATU_DonetskArtema60Building> It(GetWorld()); It; ++It) { Building = *It; break; }
    if (!District || !Building || District->GetGeneratedCollisionComponentCount() <= 0
        || Building->GetGeneratedCollisionComponentCount() <= 0) return FString();
    FString Traces;
    const FVector Locations[] = { FVector(0.f, -20000.f, 500.f), FVector(-22000.f, -20000.f, 500.f) };
    const FString ExpectedComponents[] = { TEXT("BoulevardMedian_0002"), TEXT("DistrictGround_0000") };
    const int32 ExpectedHeights[] = { 190, 120 }; // tenth-centimeters, in generator local coordinates
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FTransform Transform = District->GetActorTransform();
        const FVector Start = Transform.TransformPosition(Locations[Index]);
        FVector EndLocal = Locations[Index]; EndLocal.Z = -100.f;
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Transform.TransformPosition(EndLocal), ECC_Visibility)
            || Hit.GetActor() != District || !Hit.GetComponent() || Hit.GetComponent()->GetName() != ExpectedComponents[Index]) return FString();
        const int32 LocalHeight = FMath::RoundToInt(Transform.InverseTransformPosition(Hit.ImpactPoint).Z * 10.f);
        if (FMath::Abs(LocalHeight - ExpectedHeights[Index]) > 1) return FString();
        Traces += FString::Printf(TEXT("|%s:z%d"), *Hit.GetComponent()->GetName(), LocalHeight);
    }
    GeometrySnapshot = FString::Printf(TEXT("district=%s collision=%d building=%s collision=%d%s"),
        *District->GetGeneratedGeometrySignature(), District->GetGeneratedCollisionComponentCount(),
        *Building->GetGeneratedGeometrySignature(), Building->GetGeneratedCollisionComponentCount(), *Traces);
    UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke actual local geometry %s"), *GeometrySnapshot);
    return GeometrySnapshot;
}

void UTUExecutionNetworkProbe::ServerHello_Implementation()
{
    if (IsEnabledAuthority()) bHelloReceived = true;
}

void UTUExecutionNetworkProbe::SendCheckpoint(int32 Checkpoint)
{
    ExpectedCheckpoint = Checkpoint;
    const FString Snapshot = PawnSnapshot();
    ExpectedFingerprint = FCrc::StrCrc32(*Snapshot);
    UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke authority sent checkpoint %d fingerprint=%08x rounds=%d"),
        Checkpoint, ExpectedFingerprint, NetworkProbeRounds(ObservedPawn->ExportItemLedger()));
    ClientCheckpoint(Checkpoint, Snapshot);
}

void UTUExecutionNetworkProbe::ServerAcknowledge_Implementation(int32 Checkpoint, uint32 ObservedFingerprint)
{
    if (!IsEnabledAuthority() || Checkpoint != ExpectedCheckpoint || Checkpoint <= LastAcknowledged) return;
    if (Checkpoint < 4 && ObservedFingerprint != ExpectedFingerprint) { Fail(TEXT("client checkpoint fingerprint disagreed")); return; }
    LastAcknowledged = Checkpoint;
    ++MatchedCheckpoints;
    UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke authority acknowledged checkpoint %d"), Checkpoint);
}

void UTUExecutionNetworkProbe::ClientCheckpoint_Implementation(int32 Checkpoint, const FString& Snapshot)
{
    if (!bEnabled || GetNetMode() != NM_Client) return;
    PendingCheckpoint = Checkpoint;
    ExpectedSnapshot = Snapshot;
    bObserveOutcomes = false;
}

void UTUExecutionNetworkProbe::ClientAction_Implementation(int32 ActionNumber)
{
    if (!bEnabled || GetNetMode() != NM_Client || !ObservedPawn || ActionNumber <= LocalAction) return;
    ATU_WeaponBase* Weapon = ObservedPawn->GetPrimaryWeapon();
    if (!Weapon) { Fail(TEXT("client missing its replicated weapon")); return; }
    LocalAction = ActionNumber;
    if (ActionNumber == 1)
    {
        Weapon->StartFire();
        Weapon->StopFire();
    }
    else if (ActionNumber == 2 || ActionNumber == 3)
    {
        ReloadRequest = FGuid::NewGuid();
        ReloadRevision = Weapon->ExportItemLedger().Revision;
        Weapon->RequestReload(ReloadRequest, ReloadRevision, ETUReloadPolicy::Retain);
        if (ActionNumber == 2)
        {
            // Same normal RPC is replayed and a stale revision is separately submitted.
            Weapon->RequestReload(ReloadRequest, ReloadRevision, ETUReloadPolicy::Retain);
            Weapon->RequestReload(FGuid::NewGuid(), FMath::Max(0, ReloadRevision - 1), ETUReloadPolicy::Drop);
        }
    }
}

void UTUExecutionNetworkProbe::LocalTick()
{
    if (!ObservedPawn || !ObservedPawn->GetPrimaryWeapon() || ObservedPawn->ExportItemLedger().Weapons.IsEmpty()) return;
    if (!bHelloSent) { bHelloSent = true; ServerHello(); }
    if (LocalAction == 3)
    {
        const auto Action = ObservedPawn->GetPrimaryWeapon()->GetActionState();
        if (Action.bActive && Action.Phase == ETUWeaponActionPhase::Removed)
        {
            ObservedPawn->EquipWeaponSlot(ETUOperatorWeaponSlot::Secondary);
            LocalAction = 4;
        }
    }
    if (PendingCheckpoint < 0) return;
    bool bMatch = false;
    uint32 Fingerprint = 0;
    if (!bObserveOutcomes)
    {
        const FString Actual = PawnSnapshot();
        bMatch = Actual == ExpectedSnapshot;
        Fingerprint = FCrc::StrCrc32(*Actual);
    }
    else if (const ATU_GameState* State = GetWorld()->GetGameState<ATU_GameState>())
    {
        const auto* Host = State->Participants.FindByPredicate([this](const auto& P) { return P.PlayerId == HostId; });
        const auto* Guest = State->Participants.FindByPredicate([this](const auto& P) { return P.PlayerId == GuestId; });
        bMatch = Host && Guest && HostId != GuestId && Host->Outcome == ETURaidPlayerOutcome::Extracted
            && Host->bOutcomePersisted && Guest->Outcome == (bExpectGuestDead ? ETURaidPlayerOutcome::Dead : ETURaidPlayerOutcome::Active)
            && (!bExpectGuestDead || Guest->bOutcomePersisted);
    }
    if (bMatch)
    {
        UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke client matched actual replicated checkpoint %d"), PendingCheckpoint);
        ++MatchedCheckpoints;
        if (PendingCheckpoint == 5) bFinalOutcomesObserved = true;
        ServerAcknowledge(PendingCheckpoint, Fingerprint);
        PendingCheckpoint = -1;
    }
}

void UTUExecutionNetworkProbe::ClientObserveOutcomes_Implementation(int32 Checkpoint, FGuid HostPlayer, FGuid GuestPlayer, bool bGuestDead)
{
    if (!bEnabled || GetNetMode() != NM_Client) return;
    HostId = HostPlayer;
    GuestId = GuestPlayer;
    bExpectGuestDead = bGuestDead;
    bObserveOutcomes = true;
    PendingCheckpoint = Checkpoint;
}

void UTUExecutionNetworkProbe::ServerTick()
{
    // The owning controller/component survives production seamless HQ return.
    // Finish the receipt protocol without dereferencing the old destroyed raid pawn.
    if (ServerStage == 11 && LastAcknowledged == 5)
    {
        ClientFinish(true, TEXT("six replicated checkpoints and durable split outcomes matched"));
        ServerStage = 12;
        return;
    }
    if (ServerStage == 12)
    {
        if (bReceipt)
        {
            WriteResult(true, TEXT("guest receipt confirmed; live host persisted extraction and guest death"), TEXT("host"));
            ScheduleProcessExit(1.f);
        }
        return;
    }
    if (!bHelloReceived || !ObservedPawn || !ObservedPawn->GetPrimaryWeapon()) return;
    if (!RaidMode) RaidMode = GetWorld()->GetAuthGameMode<ATU_GameMode>();
    if (!RaidMode || RaidMode->IsTrainingRaid()) { Fail(TEXT("network probe requires live non-training authority")); return; }
    ATU_WeaponBase* Weapon = ObservedPawn->GetPrimaryWeapon();
    if (ServerStage == 0)
    {
        if (CaptureGeometry().IsEmpty()) return;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (APlayerController* Controller = It->Get(); Controller && Controller->IsLocalController())
                HostPawn = Cast<ATU_ArmedOperatorCharacter>(Controller->GetPawn());
        if (!HostPawn || !RaidMode->FindParticipant(HostPawn) || !RaidMode->FindParticipant(ObservedPawn)) return;
        HostId = RaidMode->FindParticipant(HostPawn)->PlayerId;
        GuestId = RaidMode->FindParticipant(ObservedPawn)->PlayerId;
        if (!HostId.IsValid() || !GuestId.IsValid() || HostId == GuestId) { Fail(TEXT("participants lack distinct durable IDs")); return; }
        ATU_DonetskDistrictGenerator* District = nullptr;
        for (TActorIterator<ATU_DonetskDistrictGenerator> It(GetWorld()); It; ++It) { District = *It; break; }
        if (!District) { Fail(TEXT("remote setup requires generated district")); return; }
        ATU_ArmedOperatorCharacter* Players[] = { HostPawn.Get(), ObservedPawn.Get() };
        const FVector RemoteFloors[] = { FVector(-22000,-20000,500), FVector(-22000,-18000,500) };
        FCollisionQueryParams Query(SCENE_QUERY_STAT(TUNetworkSetupFloor), false);
        Query.AddIgnoredActor(HostPawn); Query.AddIgnoredActor(ObservedPawn);
        for (int32 Index = 0; Index < 2; ++Index)
        {
            const FVector Start = District->GetActorTransform().TransformPosition(RemoteFloors[Index]);
            FHitResult Floor;
            if (!GetWorld()->LineTraceSingleByChannel(Floor, Start, Start-FVector(0,0,1000), ECC_Visibility, Query)
                || Floor.ImpactNormal.Z < .7f || Floor.GetActor() != District)
            { Fail(TEXT("remote setup lacks verified district floor")); return; }
            ATU_ArmedOperatorCharacter* Player = Players[Index];
            const FVector Destination = Floor.ImpactPoint + FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f);
            if (!Player->TeleportTo(Destination, Player->GetActorRotation(), false, false))
            { Fail(TEXT("remote setup capsule encroachment")); return; }
            Player->GetCharacterMovement()->StopMovementImmediately();
            Player->ForceNetUpdate();
            for (TActorIterator<ATU_ExtractionZone> It(GetWorld()); It; ++It)
                if (It->ContainsPawn(Player)) { Fail(TEXT("remote setup overlaps existing exit")); return; }
            for (TActorIterator<ATU_RaidCombatant> It(GetWorld()); It; ++It)
                if (It->HasLineOfSightTo(Player) || FVector::Dist(Player->GetActorLocation(), It->GetActorLocation())
                    <= FMath::Max(It->SightRangeCm, It->HearingRangeCm))
                { Fail(TEXT("remote setup inside live AI sight or hearing range")); return; }
            UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke safe setup player=%s position=%s floor=%s normalZ=%.3f outside exits and AI ranges"),
                Index == 0 ? TEXT("host") : TEXT("guest"), *Player->GetActorLocation().ToString(),
                *GetNameSafe(Floor.GetComponent()), Floor.ImpactNormal.Z);
        }
        InitialRounds = NetworkProbeRounds(ObservedPawn->ExportItemLedger());
        SendCheckpoint(0);
        ServerStage = 1;
    }
    else if (ServerStage == 1 && LastAcknowledged == 0)
    {
        ClientAction(1); ServerStage = 2; StageStarted = Elapsed;
    }
    else if (ServerStage == 2 && Elapsed - StageStarted > 1.f)
    {
        const int32 Count = NetworkProbeRounds(ObservedPawn->ExportItemLedger());
        if (Count == InitialRounds - 1) { SendCheckpoint(1); ServerStage = 3; }
        else if (Count < InitialRounds - 1) Fail(TEXT("client firing spent more than one cartridge"));
    }
    else if (ServerStage == 3 && LastAcknowledged == 1)
    {
        ReloadStartRevision = Weapon->ExportItemLedger().Revision;
        ClientAction(2); ServerStage = 4; StageStarted = Elapsed;
    }
    else if (ServerStage == 4 && Elapsed - StageStarted > 3.f && !Weapon->GetActionState().bActive
        && Weapon->GetActionState().Phase == ETUWeaponActionPhase::Ready && Weapon->ExportItemLedger().Revision > ReloadStartRevision)
    {
        if (NetworkProbeRounds(ObservedPawn->ExportItemLedger()) != InitialRounds - 1)
        { Fail(TEXT("duplicate or stale reload request changed ammunition balance")); return; }
        SendCheckpoint(2); ServerStage = 5;
    }
    else if (ServerStage == 5 && LastAcknowledged == 2)
    {
        ClientAction(3); ServerStage = 6; StageStarted = Elapsed;
    }
    else if (ServerStage == 6 && Elapsed - StageStarted > 3.f
        && ObservedPawn->GetActiveWeaponSlot() == ETUOperatorWeaponSlot::Secondary && !Weapon->GetActionState().bActive)
    {
        if (Weapon->GetActionState().Phase == ETUWeaponActionPhase::Ready)
        { Fail(TEXT("network slot switch arrived after reload completed; interruption not proven")); return; }
        if (NetworkProbeRounds(ObservedPawn->ExportItemLedger()) != InitialRounds - 1)
        { Fail(TEXT("interrupted network reload changed ammunition balance")); return; }
        SendCheckpoint(3); ServerStage = 7;
    }
    else if (ServerStage == 7 && LastAcknowledged == 3)
    {
        if (FVector::Dist2D(HostPawn->GetActorLocation(), ObservedPawn->GetActorLocation()) < 1000.f)
        { Fail(TEXT("players lost safe extraction separation")); return; }
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ATU_ExtractionZone* Exit = GetWorld()->SpawnActor<ATU_ExtractionZone>(ATU_ExtractionZone::StaticClass(), HostPawn->GetActorLocation(), FRotator::ZeroRotator, Spawn);
        if (!Exit) { Fail(TEXT("could not spawn host extraction zone")); return; }
        Exit->ExtractId = TEXT("QA_Network_HostExit"); Exit->HoldSeconds = 1.f;
        UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke exit bounds hostInside=%d guestInside=%d hostOutcome=%d guestOutcome=%d"),
            Exit->ContainsPawn(HostPawn), Exit->ContainsPawn(ObservedPawn),
            static_cast<int32>(RaidMode->FindParticipant(HostPawn)->Outcome), static_cast<int32>(RaidMode->FindParticipant(ObservedPawn)->Outcome));
        if (!Exit->ContainsPawn(HostPawn) || Exit->ContainsPawn(ObservedPawn))
        { Fail(TEXT("exit must contain host and exclude active guest")); return; }
        if (!RaidMode->BeginExtraction(HostPawn, Exit)) { Fail(TEXT("host extraction eligibility failed")); return; }
        ServerStage = 8;
    }
    else if (ServerStage == 8)
    {
        const auto* Host = RaidMode->FindParticipant(HostPawn);
        const auto* Guest = RaidMode->FindParticipant(ObservedPawn);
        if (Host && Guest && Host->Outcome == ETURaidPlayerOutcome::Extracted && Host->bOutcomePersisted)
        {
            if (Guest->Outcome != ETURaidPlayerOutcome::Active)
            {
                UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke unexpected split host=%d guest=%d guestExtract=%s"),
                    static_cast<int32>(Host->Outcome), static_cast<int32>(Guest->Outcome), *Guest->ExtractId.ToString());
                Fail(TEXT("host extraction ended guest raid")); return;
            }
            ExpectedCheckpoint = 4;
            ClientObserveOutcomes(4, HostId, GuestId, false);
            ServerStage = 9;
        }
    }
    else if (ServerStage == 9 && LastAcknowledged == 4)
    {
        UTUHealthComponent* Health = ObservedPawn->FindComponentByClass<UTUHealthComponent>();
        if (!Health) { Fail(TEXT("guest lacks real regional health")); return; }
        Health->ApplyRegionalDamage(ETUBodyRegion::Head, 100000.f);
        ServerStage = 10;
    }
    else if (ServerStage == 10)
    {
        const auto* Guest = RaidMode->FindParticipant(ObservedPawn);
        if (!Guest || Guest->Outcome != ETURaidPlayerOutcome::Dead || !Guest->bOutcomePersisted) return;
        UTUHideoutLifecycleSubsystem* Life = GetWorld()->GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>();
        if (!Life || Life->GetPlayerStash(HostId).Weapons.IsEmpty() || !Life->GetPlayerStash(GuestId).Weapons.IsEmpty())
        { Fail(TEXT("durable host extraction or guest loss inventory mismatch")); return; }
        const auto& Outcomes = Life->GetProfile()->Outcomes;
        const bool bHostStored = Outcomes.ContainsByPredicate([this](const auto& V) { return V.PlayerId == HostId && V.Outcome == ETURaidPlayerOutcome::Extracted; });
        const bool bGuestStored = Outcomes.ContainsByPredicate([this](const auto& V) { return V.PlayerId == GuestId && V.Outcome == ETURaidPlayerOutcome::Dead; });
        if (!bHostStored || !bGuestStored) { Fail(TEXT("disk authority lacks both split outcomes")); return; }
        ExpectedCheckpoint = 5;
        ClientObserveOutcomes(5, HostId, GuestId, true); ServerStage = 11;
    }
}

void UTUExecutionNetworkProbe::ClientFinish_Implementation(bool bPassed, const FString& Reason)
{
    if (!bEnabled || GetNetMode() != NM_Client) return;
    WriteResult(bPassed, Reason, TEXT("guest"));
    ServerFinishReceipt();
    ScheduleProcessExit(2.f);
}

void UTUExecutionNetworkProbe::ServerFinishReceipt_Implementation()
{
    if (IsEnabledAuthority() && ServerStage == 12) bReceipt = true;
}

void UTUExecutionNetworkProbe::Fail(const FString& Reason)
{
    if (bFinished) return;
    if (IsValid(RaidMode))
    {
        ATU_ArmedOperatorCharacter* Players[] = { HostPawn.Get(), ObservedPawn.Get() };
        for (ATU_ArmedOperatorCharacter* Player : Players)
        {
            if (!IsValid(Player)) continue;
            const auto* Participant = RaidMode->FindParticipant(Player);
            const UTUHealthComponent* Health = Player->FindComponentByClass<UTUHealthComponent>();
            UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke failure player=%s outcome=%d extract=%s health=%.2f position=%s"),
                Player == HostPawn ? TEXT("host") : TEXT("guest"), Participant ? static_cast<int32>(Participant->Outcome) : -1,
                Participant ? *Participant->ExtractId.ToString() : TEXT("missing"), Health ? Health->GetTotalHealth() : -1.f,
                *Player->GetActorLocation().ToString());
        }
    }
    if (IsEnabledAuthority())
    {
        if (const APlayerController* Controller = Cast<APlayerController>(GetOwner()); Controller && !Controller->IsLocalController())
            ClientFinish(false, Reason);
    }
    WriteResult(false, Reason, GetNetMode() == NM_Client ? TEXT("guest") : TEXT("host"));
    ScheduleProcessExit(2.f);
}

void UTUExecutionNetworkProbe::WriteResult(bool bPassed, const FString& Reason, const FString& Role)
{
    if (bFinished) return;
    FNetworkRunVerdict& Verdict = NetworkRunVerdicts.FindOrAdd(RunId);
    if (Verdict.bFinal) { bFinished = true; return; }
    Verdict.bFinal = true;
    if (Verdict.Role.IsEmpty()) Verdict.Role = ProcessRole.IsEmpty() ? Role : ProcessRole;
    const FString FinalRole = Verdict.Role;
    bFinished = true; bPassedResult = bPassed;
    int32 Lag = -1, Loss = -1, IncomingLag = -1, IncomingLoss = -1;
#if DO_ENABLE_NET_TEST
    if (const UNetDriver* Driver = GetWorld()->GetNetDriver())
    {
        Lag = Driver->PacketSimulationSettings.PktLag;
        Loss = Driver->PacketSimulationSettings.PktLoss;
        IncomingLag = Driver->PacketSimulationSettings.PktIncomingLagMax;
        IncomingLoss = Driver->PacketSimulationSettings.PktIncomingLoss;
    }
#endif
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TUNetworkSmoke"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Json = FString::Printf(TEXT("{\"passed\":%s,\"role\":\"%s\",\"profile\":\"%s\",\"checkpoints\":%d,\"hostPlayer\":\"%s\",\"guestPlayer\":\"%s\",\"stage\":%d,\"outgoingLagMs\":%d,\"outgoingLossPercent\":%d,\"incomingLagMs\":%d,\"incomingLossPercent\":%d,\"elapsedSeconds\":%.3f,\"reason\":\"%s\",\"expectedCheckpoint\":%d,\"acknowledgedCheckpoint\":%d,\"pendingCheckpoint\":%d,\"geometry\":\"%s\"}\n"),
        bPassed ? TEXT("true") : TEXT("false"), *FinalRole, *ProfileName, MatchedCheckpoints, *HostId.ToString(), *GuestId.ToString(),
        ServerStage, Lag, Loss, IncomingLag, IncomingLoss, Elapsed, *Reason,
        ExpectedCheckpoint, LastAcknowledged, PendingCheckpoint, *GeometrySnapshot);
    const FString Path = FPaths::Combine(Directory, RunId + TEXT(".") + FinalRole + TEXT(".json"));
    if (!FFileHelper::SaveStringToFile(Json, *Path)) bPassedResult = false;
    Verdict.bPassed = bPassedResult;
    UE_LOG(LogTemp, Display, TEXT("TUNetworkSmoke %s %s: %s evidence=%s"), *FinalRole, bPassedResult ? TEXT("PASS") : TEXT("FAIL"), *Reason, *Path);
}

void UTUExecutionNetworkProbe::ScheduleProcessExit(float DelaySeconds)
{
    FNetworkRunVerdict& Verdict = NetworkRunVerdicts.FindOrAdd(RunId);
    if (!Verdict.bFinal || Verdict.bExitScheduled) return;
    Verdict.bExitScheduled = true;
    const uint8 ExitCode = Verdict.bPassed ? 0 : 1;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([ExitCode](float)
    {
        FPlatformMisc::RequestExitWithStatus(false, ExitCode);
        return false;
    }), DelaySeconds);
}
