#include "TUExecutionSmokeActor.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "TUHealthComponent.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_ExtractionZone.h"
#include "TU_GameMode.h"
#include "TU_WeaponBase.h"
#include "TU_CommandCenterStation.h"
#include "TU_HideoutGameMode.h"
#include "TUMissionPackageData.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
    struct FSmokeJourney
    {
        int32 State = 0; // 0: HQ launch, 1: deployed raid, 2: production return awaited
        FGuid RaidId;
        FString ExpectedProfile;
        FString ExpectedKit;
        int32 Shots = 0;
        float TotalSeconds = 0.f;
    };
    TMap<TWeakObjectPtr<UGameInstance>, FSmokeJourney> SmokeJourneys;

    FString SmokeKitPayload(FTUItemLedger Ledger)
    {
        // Hydration rebases revisions; physical payload and interrupted phase must persist.
        Ledger.Revision = 0;
        for (auto& Action : Ledger.WeaponActions) { Action.Revision = 0; Action.PhaseEndServerTime = 0.f; }
        FString Text;
        FTUItemLedger::StaticStruct()->ExportText(Text, &Ledger, nullptr, nullptr, PPF_None, nullptr);
        return Text;
    }

    int32 SmokeRounds(const FTUItemLedger& Ledger)
    {
        int32 Total = Ledger.LooseCartridges.Num();
        for (const auto& W : Ledger.Weapons) Total += W.ChamberAmmoId.IsNone() ? 0 : 1;
        for (const auto& M : Ledger.Magazines) Total += M.Cartridges.Num();
        return Total;
    }

    template<typename T> void AppendSmokeStruct(FString& Text, const T& Value)
    {
        FString Entry;
        T::StaticStruct()->ExportText(Entry, &Value, nullptr, nullptr, PPF_None, nullptr);
        Text += Entry + TEXT("\n");
    }
}

ATUExecutionSmokeActor::ATUExecutionSmokeActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ATUExecutionSmokeActor::BeginPlay()
{
    Super::BeginPlay();
#if UE_BUILD_SHIPPING
    Destroy();
    return;
#else
    FParse::Value(FCommandLine::Get(), TEXT("TUExecutionSmoke="), Mode);
    FParse::Value(FCommandLine::Get(), TEXT("TUProfileSlot="), ProfileSlot);
    bRoundTrip = Mode == TEXT("roundtrip-extract") || Mode == TEXT("roundtrip-death");
    bDeathScenario = Mode == TEXT("death") || Mode == TEXT("roundtrip-death");
    FParse::Value(FCommandLine::Get(), TEXT("TUWarmupSeconds="), WarmupSeconds);
    WarmupSeconds = FMath::IsFinite(WarmupSeconds) ? FMath::Clamp(WarmupSeconds, 0.f, 60.f) : 0.f;
    bTakeScreenshot = FParse::Param(FCommandLine::Get(), TEXT("TUScreenshot"));
    if (bTakeScreenshot) WarmupSeconds = FMath::Max(WarmupSeconds, 10.f);
    if (Mode == TEXT("spawn")) WarmupSeconds = FMath::Max(WarmupSeconds, 10.f);
    bool bSafeSlot = ProfileSlot.StartsWith(TEXT("TU_Automation_"));
    for (const TCHAR Character : ProfileSlot) bSafeSlot &= FChar::IsAlnum(Character) || Character == TCHAR('_');
    if (!bSafeSlot || (!bRoundTrip && Mode != TEXT("extract") && Mode != TEXT("death") && Mode != TEXT("verify") && Mode != TEXT("spawn")))
    {
        Finish(false, TEXT("requires valid mode and isolated TU_Automation_ profile"));
        return;
    }
    EvidenceBase = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TUExecutionSmoke"), ProfileSlot);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidenceBase), true);
    Lifecycle = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>() : nullptr;
    RaidMode = GetWorld()->GetAuthGameMode<ATU_GameMode>();
    if (!Lifecycle || !Lifecycle->GetProfile() || !RaidMode)
    {
        Finish(false, TEXT("missing live lifecycle or raid mode"));
        return;
    }
    if (Mode == TEXT("verify"))
    {
        FString Expected;
        const bool bRead = FFileHelper::LoadFileToString(Expected, *(EvidenceBase + TEXT(".expected.txt")));
        Finish(bRead && Expected == Snapshot(Lifecycle->GetProfile()), TEXT("separate-process persisted profile comparison"));
    }
    else if (bRoundTrip)
    {
        for (auto It = SmokeJourneys.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
        FSmokeJourney& Journey = SmokeJourneys.FindOrAdd(GetGameInstance());
        if (RaidMode->IsA<ATU_HideoutGameMode>())
        {
            if (Journey.State == 0) Stage = -1;
            else if (Journey.State == 2) { Stage = 5; Shots = Journey.Shots; }
            else Finish(false, TEXT("HQ reached without a completed raid journey"));
        }
        else if (Journey.State != 1 || !Journey.RaidId.IsValid() || Journey.RaidId != Lifecycle->GetActiveRaidId()
            || !GetWorld()->GetMapName().Contains(TEXT("Donetsk")))
            Finish(false, TEXT("raid travel did not preserve deployed raid identity"));
        else UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke roundtrip entered actual raid %s"), *Journey.RaidId.ToString());
    }
#endif
    if (!bFinished && Mode != TEXT("verify")) IsolatePhysicalInput();
}

FString ATUExecutionSmokeActor::Snapshot(const UTUHideoutSaveGame* Profile) const
{
    if (!Profile) return TEXT("missing profile");
    FString Text = FString::Printf(TEXT("Schema=%d Player=%s Completed=%d\n"),
        Profile->SaveVersion, *Profile->LocalPlayerId.ToString(), Profile->CompletedOperations);
    FTUItemLedger Ledger = Profile->Stash;
    // Cosmetic deadlines are deliberately not durable save data.
    for (auto& Action : Ledger.WeaponActions) Action.PhaseEndServerTime = 0.f;
    AppendSmokeStruct(Text, Ledger);
    TArray<FGuid> LooseOwners;
    Profile->LooseByPlayer.GetKeys(LooseOwners);
    LooseOwners.Sort([](const FGuid& A, const FGuid& B) { return A.ToString() < B.ToString(); });
    for (const FGuid& LedgerOwner : LooseOwners)
    {
        Text += TEXT("LooseOwner=") + LedgerOwner.ToString() + TEXT("\n");
        AppendSmokeStruct(Text, Profile->LooseByPlayer.FindChecked(LedgerOwner));
    }
    for (const auto& Task : Profile->Tasks) AppendSmokeStruct(Text, Task);
    for (const auto& Outcome : Profile->Outcomes) AppendSmokeStruct(Text, Outcome);
    for (auto Deployment : Profile->Deployments)
    {
        for (auto& Action : Deployment.Escrow.WeaponActions) Action.PhaseEndServerTime = 0.f;
        AppendSmokeStruct(Text, Deployment);
    }
    return Text;
}

bool ATUExecutionSmokeActor::WarmupReady()
{
    if (PawnReadyTime < 0.f) PawnReadyTime = Elapsed;
    if (Elapsed - PawnReadyTime < WarmupSeconds) return false;
    if (bTakeScreenshot && !bScreenshotRequested)
    {
        bScreenshotRequested = true;
        if (Operator)
        {
            APlayerController* PC = Cast<APlayerController>(Operator->GetController());
            if (PC && PC->PlayerCameraManager)
            {
                const FVector CameraPosition = PC->PlayerCameraManager->GetCameraLocation();
                const FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
                UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke view camera=%s rotation=%s FOV=%.2f pawn=%s capsuleHalf=%.2f bodyOrigin=%s bodyExtent=%s head=%s"),
                    *CameraPosition.ToString(), *CameraRotation.ToString(), PC->PlayerCameraManager->GetFOVAngle(),
                    *Operator->GetActorTransform().ToString(), Operator->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
                    *Operator->GetMesh()->Bounds.Origin.ToString(), *Operator->GetMesh()->Bounds.BoxExtent.ToString(),
                    *Operator->GetMesh()->GetBoneLocation(TEXT("head")).ToString());
                if (ATU_WeaponBase* Weapon = Operator->GetPrimaryWeapon())
                    UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke view weapon=%s bodyOrigin=%s bodyExtent=%s"),
                        *Weapon->GetActorTransform().ToString(), *Weapon->GetWeaponBodyMesh()->Bounds.Origin.ToString(),
                        *Weapon->GetWeaponBodyMesh()->Bounds.BoxExtent.ToString());
                FCollisionQueryParams Query(SCENE_QUERY_STAT(TUSmokeCamera), false, Operator);
                if (Operator->GetPrimaryWeapon()) Query.AddIgnoredActor(Operator->GetPrimaryWeapon());
                FHitResult Forward, Ground;
                GetWorld()->LineTraceSingleByChannel(Forward, CameraPosition, CameraPosition+CameraRotation.Vector()*100000.f, ECC_Visibility, Query);
                GetWorld()->LineTraceSingleByChannel(Ground, CameraPosition, CameraPosition-FVector(0,0,10000), ECC_Visibility, Query);
                const bool bCameraBlocked = GetWorld()->OverlapBlockingTestByChannel(CameraPosition, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(2.f), Query);
                UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke view forward=%s component=%s distance=%.2f ground=%s component=%s distance=%.2f cameraCollision=%d"),
                    *GetNameSafe(Forward.GetActor()), *GetNameSafe(Forward.GetComponent()), Forward.Distance,
                    *GetNameSafe(Ground.GetActor()), *GetNameSafe(Ground.GetComponent()), Ground.Distance, bCameraBlocked);
            }
        }
        const FString ScreenshotPath = EvidenceBase + TEXT(".") + GetWorld()->GetMapName() + TEXT(".warmup.png");
        FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
        UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke warmed pawn %.1fs; screenshot requested %s"), WarmupSeconds, *ScreenshotPath);
    }
    return true;
}

void ATUExecutionSmokeActor::IsolatePhysicalInput()
{
    ATU_ArmedOperatorCharacter* Player = Cast<ATU_ArmedOperatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (!PC || InputIsolatedPawn.Get() == Player) return;
    // This opt-in driver owns action intent. Disable only OS input dispatch;
    // direct gameplay requests still run every production authority/weapon gate.
    if (PC->PlayerInput) PC->PlayerInput->FlushPressedKeys();
    Player->DisableInput(PC);
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    InputIsolatedPawn = Player;
    Operator = Player;
    UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke input isolated map=%s pawn=%s control=%s"),
        *GetWorld()->GetMapName(), *Player->GetActorTransform().ToString(), *PC->GetControlRotation().ToString());
    LogWeaponState(TEXT("initial baseline"));
}

void ATUExecutionSmokeActor::LogWeaponState(const TCHAR* Label) const
{
    if (!Operator) return;
    const ATU_WeaponBase* Weapon = Operator->GetPrimaryWeapon();
    const UTUHealthComponent* Health = Operator->FindComponentByClass<UTUHealthComponent>();
    const FTURaidParticipantState* Participant = RaidMode ? RaidMode->FindParticipant(Operator) : nullptr;
    UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke weapon %s stage=%d shots=%d slot=%d current=%s primary=%s phase=%d actionActive=%d health=%.2f outcome=%d raised=%d disabled=%d ammo=%d total=%d obstructed=%d canFire=%d position=%s"),
        Label, Stage, Shots, static_cast<int32>(Operator->GetActiveWeaponSlot()), *GetNameSafe(Operator->GetCurrentWeapon()),
        *GetNameSafe(Weapon), Weapon ? static_cast<int32>(Weapon->GetActionState().Phase) : -1,
        Weapon && Weapon->GetActionState().bActive, Health ? Health->GetTotalHealth() : -1.f,
        Participant ? static_cast<int32>(Participant->Outcome) : -1, Operator->IsWeaponRaised(), Operator->IsCombatDisabled(),
        Weapon ? Weapon->GetCurrentAmmo() : -1, SmokeRounds(Operator->ExportItemLedger()),
        Weapon && Weapon->IsMuzzleObstructed(), Weapon && Weapon->CanFire(), *Operator->GetActorLocation().ToString());
}

void ATUExecutionSmokeActor::TickSpawnCheck()
{
    if (Elapsed > WarmupSeconds + 20.f) { Finish(false, TEXT("idle spawn possession or settling deadline exceeded")); return; }
    APlayerController* PC = Operator ? Cast<APlayerController>(Operator->GetController()) : nullptr;
    if (!PC || PC->GetPawn() != Operator || !PC->PlayerCameraManager) return;
    if (PawnReadyTime < 0.f) PawnReadyTime = Elapsed;
    const float IdleSeconds = Elapsed - PawnReadyTime;
    if (IdleSeconds < 2.f) return; // Permit ordinary spawn gravity to settle onto the floor.
    const FVector Position = Operator->GetActorLocation();
    const FVector CameraPosition = PC->PlayerCameraManager->GetCameraLocation();
    const float HalfHeight = Operator->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const UCharacterMovementComponent* Movement = Operator->GetCharacterMovement();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TUSmokeSpawnSupport), false, Operator);
    if (Operator->GetPrimaryWeapon()) Query.AddIgnoredActor(Operator->GetPrimaryWeapon());
    if (Operator->GetSecondaryWeapon()) Query.AddIgnoredActor(Operator->GetSecondaryWeapon());
    FHitResult Support;
    const bool bSupport = GetWorld()->LineTraceSingleByChannel(Support, Position,
        Position-FVector(0,0,HalfHeight+30.f), ECC_Visibility, Query) && Support.ImpactNormal.Z >= .7f;
    SpawnCameraDistance = FVector::Distance(Position, CameraPosition);
    SpawnSupportDistance = Support.Distance;
    if (SpawnGroundedSamples == 0) SpawnMinHeight = SpawnMaxHeight = Position.Z;
    SpawnMinHeight = FMath::Min(SpawnMinHeight, static_cast<float>(Position.Z));
    SpawnMaxHeight = FMath::Max(SpawnMaxHeight, static_cast<float>(Position.Z));
    const bool bCameraSane = !Position.ContainsNaN() && !CameraPosition.ContainsNaN()
        && !PC->PlayerCameraManager->GetCameraRotation().ContainsNaN()
        && FMath::IsFinite(PC->PlayerCameraManager->GetFOVAngle())
        && PC->PlayerCameraManager->GetFOVAngle() >= 30.f && PC->PlayerCameraManager->GetFOVAngle() <= 130.f
        && SpawnCameraDistance <= HalfHeight+50.f;
    const bool bGrounded = Movement && Movement->IsMovingOnGround() && !Movement->IsFalling()
        && FMath::Abs(Movement->Velocity.Z) <= 5.f && SpawnMaxHeight-SpawnMinHeight <= 10.f;
    if (!bSupport || !bGrounded || !bCameraSane)
    {
        UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke idle spawn FAIL idle=%.2f support=%s distance=%.2f normalZ=%.3f grounded=%d cameraSane=%d position=%s camera=%s velocity=%s heightRange=%.2f"),
            IdleSeconds, *GetNameSafe(Support.GetComponent()), Support.Distance, Support.ImpactNormal.Z, bGrounded, bCameraSane,
            *Position.ToString(), *CameraPosition.ToString(), Movement ? *Movement->Velocity.ToString() : TEXT("missing"), SpawnMaxHeight-SpawnMinHeight);
        Finish(false, TEXT("idle possessed spawn failed floor grounded height or camera validation")); return;
    }
    ++SpawnGroundedSamples;
    if (IdleSeconds < WarmupSeconds) return;
    if (!WarmupReady()) return;
    // Leave a rendered frame after a screenshot request before exiting.
    if (IdleSeconds < WarmupSeconds+1.f) return;
    UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke idle spawn PASS map=%s idle=%.2f samples=%d heightRange=%.2f support=%s distance=%.2f cameraDistance=%.2f"),
        *GetWorld()->GetMapName(), IdleSeconds, SpawnGroundedSamples, SpawnMaxHeight-SpawnMinHeight,
        *GetNameSafe(Support.GetComponent()), SpawnSupportDistance, SpawnCameraDistance);
    Finish(true, TEXT("normal possessed spawn remained grounded with stable height supported floor and sane camera"));
}

void ATUExecutionSmokeActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished || Mode == TEXT("verify")) return;
    IsolatePhysicalInput();
    Elapsed += DeltaSeconds;
    if (Mode == TEXT("spawn")) { TickSpawnCheck(); return; }
    if (bRoundTrip) SmokeJourneys.FindChecked(GetGameInstance()).TotalSeconds += DeltaSeconds;
    if (Elapsed > WarmupSeconds + 45.f) { Finish(false, TEXT("gameplay deadline exceeded after configured warmup")); return; }
    if (Stage == -1)
    {
        Operator = Cast<ATU_ArmedOperatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        if (!Operator || !Operator->GetPrimaryWeapon()
            || (Lifecycle->GetProfile()->bInitialKitCaptured && !Operator->HasHydratedInventory())) return;
        if (!WarmupReady()) return;
        for (TActorIterator<ATU_CommandCenterStation> It(GetWorld()); It; ++It)
        {
            const UTUMissionPackageData* Package = It->GetMissionPackage();
            if (It->GetStationType() != ETUCommandCenterStationType::MissionLaunch || !Package
                || Package->bTrainingOnly || Package->Mission.MissionId != TEXT("OP_RELAY_RECOVERY")) continue;
            const FVector Approach = It->GetActorLocation() - It->GetActorForwardVector() * 200.f;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(TUSmokeStationFloor), false, Operator);
            Query.AddIgnoredActor(*It);
            FHitResult Floor;
            if (!GetWorld()->LineTraceSingleByChannel(Floor, Approach+FVector(0,0,300), Approach-FVector(0,0,500), ECC_Visibility, Query)
                || Floor.ImpactNormal.Z < .7f)
            { Finish(false, TEXT("mission station approach lacks supporting floor")); return; }
            const FVector Destination = Floor.ImpactPoint+FVector(0,0,Operator->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f);
            if (!Operator->TeleportTo(Destination, Operator->GetActorRotation(), false, false))
            { Finish(false, TEXT("mission station approach capsule encroachment")); return; }
            if (!It->IsOperatorInRange(Operator)) { Finish(false, TEXT("operator could not reach actual mission station")); return; }
            FSmokeJourney& Journey = SmokeJourneys.FindChecked(GetGameInstance());
            Journey.State = 1;
            if (!It->UseStation(Operator)) { Finish(false, TEXT("actual mission launch station rejected deployment")); return; }
            Journey.RaidId = Lifecycle->GetActiveRaidId();
            if (!Journey.RaidId.IsValid()) { Finish(false, TEXT("station deployment lacked durable escrow identity")); return; }
            Stage = 4; // Wait for production OpenLevel; never manually travel the driver.
            UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke roundtrip station deployed raid %s"), *Journey.RaidId.ToString());
            return;
        }
        return;
    }
    if (Stage == 5)
    {
        Operator = Cast<ATU_ArmedOperatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        if (!Operator || !Operator->HasHydratedInventory()) return;
        const FSmokeJourney& Journey = SmokeJourneys.FindChecked(GetGameInstance());
        if (!RaidMode->IsA<ATU_HideoutGameMode>() || Lifecycle->GetActiveRaidId().IsValid() || Lifecycle->IsMissionInProgress())
        { Finish(false, TEXT("production return did not restore inactive HQ lifecycle")); return; }
        if (Snapshot(Lifecycle->GetProfile()) != Journey.ExpectedProfile
            || SmokeKitPayload(Operator->ExportItemLedger()) != Journey.ExpectedKit)
        { Finish(false, TEXT("HQ pawn hydration differs from durable post-raid inventory")); return; }
        Elapsed = Journey.TotalSeconds;
        if (!FFileHelper::SaveStringToFile(Journey.ExpectedProfile, *(EvidenceBase + TEXT(".expected.txt"))))
        { Finish(false, TEXT("could not write roundtrip relaunch evidence")); return; }
        Finish(true, TEXT("actual HQ station raid and production HQ return preserved kit and outcome"));
        return;
    }
    if (Stage == 0)
    {
        Operator = Cast<ATU_ArmedOperatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        if (!Operator || !Operator->GetPrimaryWeapon()) return;
        if (!WarmupReady()) return;
        if (!RaidMode->RegisterParticipant(Operator).IsValid()) { Finish(false, TEXT("participant registration failed")); return; }
        const FTURaidParticipantState* Participant = RaidMode->FindParticipant(Operator);
        if (!Participant || Participant->Outcome != ETURaidPlayerOutcome::Active || RaidMode->IsTrainingRaid())
        { Finish(false, TEXT("smoke requires active non-training raid")); return; }
        FTUTaskDefinition Task;
        Task.TaskId = TEXT("QA_Smoke_Unfinished");
        Task.Condition = ETUTaskCondition::Interact;
        Task.TargetId = TEXT("QA_Smoke_Object");
        Task.Policy = ETUTaskPolicy::Cumulative;
        Task.RequiredCount = 3;
        if (!RaidMode->AddTask(Operator, Task)
            || !RaidMode->RecordUniqueTaskEvent(Operator, FGuid::NewGuid(), Task.Condition, Task.TargetId, 1)
            || !RaidMode->RecoverItem(Operator, TEXT("QA_Smoke_Loot"), FGuid::NewGuid()))
        { Finish(false, TEXT("task or loot gameplay transaction failed")); return; }
        InitialRounds = SmokeRounds(Operator->ExportItemLedger());
        Stage = 1;
    }
    else if (Stage == 1)
    {
        if (Elapsed - LastShotTime < 0.25f) return;
        ATU_WeaponBase* Weapon = Operator->GetPrimaryWeapon();
        const int32 Before = Weapon->GetCurrentAmmo();
        LogWeaponState(TEXT("before driver shot"));
        Weapon->StartFire();
        Weapon->StopFire();
        if (Weapon->GetCurrentAmmo() != Before - 1)
        {
            LogWeaponState(TEXT("shot rejected or unexpected consumption"));
            UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke shot delta before=%d after=%d"), Before, Weapon->GetCurrentAmmo());
            Finish(false, TEXT("actual weapon shot did not consume exactly one cartridge")); return;
        }
        LastShotTime = Elapsed;
        if (++Shots == 11)
        {
            Weapon->StartReload();
            if (!Weapon->GetActionState().bActive) { Finish(false, TEXT("reload did not begin")); return; }
            Stage = 2;
        }
    }
    else if (Stage == 2)
    {
        ATU_WeaponBase* Weapon = Operator->GetPrimaryWeapon();
        if (Weapon->GetActionState().Phase != ETUWeaponActionPhase::Removed) return;
        if (!Operator->EquipWeaponSlot(ETUOperatorWeaponSlot::Secondary) || Weapon->GetActionState().bActive)
        { Finish(false, TEXT("slot switch failed to interrupt removed-magazine phase")); return; }
        if (SmokeRounds(Operator->ExportItemLedger()) != InitialRounds - Shots)
        { Finish(false, TEXT("reload interruption violated ammunition conservation")); return; }
        if (bDeathScenario)
        {
            UTUHealthComponent* Health = Operator->FindComponentByClass<UTUHealthComponent>();
            if (!Health) { Finish(false, TEXT("player has no actual health component")); return; }
            Health->ApplyRegionalDamage(ETUBodyRegion::Head, 100000.f);
        }
        else
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            ATU_ExtractionZone* Exit = GetWorld()->SpawnActor<ATU_ExtractionZone>(ATU_ExtractionZone::StaticClass(), Operator->GetActorLocation(), FRotator::ZeroRotator, Params);
            if (!Exit) { Finish(false, TEXT("could not create actual extraction zone")); return; }
            Exit->ExtractId = TEXT("QA_Smoke_Exit");
            Exit->HoldSeconds = 1.f;
            if (!RaidMode->BeginExtraction(Operator, Exit)) { Finish(false, TEXT("eligible extraction countdown refused")); return; }
        }
        Stage = 3;
    }
    else if (Stage == 3)
    {
        const FTURaidParticipantState* Participant = RaidMode->FindParticipant(Operator);
        if (!Participant || !Participant->bOutcomePersisted) return;
        const ETURaidPlayerOutcome Expected = bDeathScenario ? ETURaidPlayerOutcome::Dead : ETURaidPlayerOutcome::Extracted;
        if (Participant->Outcome != Expected) { Finish(false, TEXT("unexpected terminal player outcome")); return; }
        const FTUTaskProgress* Task = Participant->Tasks.FindByPredicate([](const auto& Value) { return Value.Definition.TaskId == TEXT("QA_Smoke_Unfinished"); });
        if (!Task || Task->bCompleted || Task->CommittedCount != 1)
        { Finish(false, TEXT("unfinished cumulative task lost or incorrectly completed")); return; }
        const FString State = Snapshot(Lifecycle->GetProfile());
        if (bRoundTrip)
        {
            FSmokeJourney& Journey = SmokeJourneys.FindChecked(GetGameInstance());
            Journey.ExpectedProfile = State;
            Journey.ExpectedKit = SmokeKitPayload(Lifecycle->GetPlayerStash(Lifecycle->GetLocalPlayerId()));
            Journey.Shots = Shots;
            Journey.State = 2;
            Stage = 4;
            UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke roundtrip durable outcome; awaiting production HQ travel"));
            return;
        }
        if (!FFileHelper::SaveStringToFile(State, *(EvidenceBase + TEXT(".expected.txt"))))
        { Finish(false, TEXT("could not write expected relaunch evidence")); return; }
        Finish(true, TEXT("live pawn weapon interruption task loot and durable outcome executed"));
    }
}

void ATUExecutionSmokeActor::Finish(bool bPassed, const FString& Reason)
{
    if (bFinished) return;
    bFinished = true;
    const uint32 Fingerprint = Lifecycle ? FCrc::StrCrc32(*Snapshot(Lifecycle->GetProfile())) : 0;
    const FString Json = FString::Printf(TEXT("{\"passed\":%s,\"mode\":\"%s\",\"shots\":%d,\"elapsedSeconds\":%.3f,\"profileFingerprint\":\"%08x\",\"reason\":\"%s\",\"map\":\"%s\",\"groundedSamples\":%d,\"heightRangeCm\":%.3f,\"supportDistanceCm\":%.3f,\"cameraDistanceCm\":%.3f}\n"),
        bPassed ? TEXT("true") : TEXT("false"), *Mode, Shots, Elapsed, Fingerprint, *Reason, *GetWorld()->GetMapName(), SpawnGroundedSamples, SpawnMaxHeight-SpawnMinHeight, SpawnSupportDistance, SpawnCameraDistance);
    if (!EvidenceBase.IsEmpty()) FFileHelper::SaveStringToFile(Json, *(EvidenceBase + TEXT(".") + Mode + TEXT(".json")));
    UE_LOG(LogTemp, Display, TEXT("TUExecutionSmoke %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *Reason);
    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
