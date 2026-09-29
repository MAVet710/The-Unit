#include "TU_ArmedOperatorCharacter.h"

#include "TUArmoryWidget.h"
#include "TUBriefingWidget.h"
#include "TUMeleeLoadoutComponent.h"
#include "TU_CommandCenterStation.h"
#include "TU_OTFKnife.h"
#include "TU_TacticalRifle.h"
#include "TU_WeaponBase.h"
#include "TU_GameMode.h"
#include "TU_InteractableBase.h"
#include "TUHealthComponent.h"
#include "TUWorldItem.h"
#include "TUWeaponPresentationComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATU_ArmedOperatorCharacter::ATU_ArmedOperatorCharacter()
{
    bReplicates = true;
    DefaultWeaponClass = ATU_TacticalRifle::StaticClass();
    DefaultMeleeClass = ATU_OTFKnife::StaticClass();
    OperatorLoadout = CreateDefaultSubobject<UTUOperatorLoadoutComponent>(TEXT("OperatorLoadout"));
    MeleeLoadout = CreateDefaultSubobject<UTUMeleeLoadoutComponent>(TEXT("MeleeLoadout"));
    ArmoryWidgetClass = UTUArmoryWidget::StaticClass();
    BriefingWidgetClass = UTUBriefingWidget::StaticClass();

    MX50ChestVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MX50ChestVisual"));
    MX50ChestVisual->SetupAttachment(GetMesh(), MX50ChestSocket);
    MX50ChestVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MX50ChestVisual->SetOwnerNoSee(true);

    MX50FirstPersonVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MX50FirstPersonVisual"));
    MX50FirstPersonVisual->SetupAttachment(FirstPersonCamera);
    MX50FirstPersonVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MX50FirstPersonVisual->SetOnlyOwnerSee(true);
    MX50FirstPersonVisual->SetHiddenInGame(true);
    MX50FirstPersonVisual->SetRelativeTransform(MX50RaisedTransform);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        // Temporary rugged-tablet silhouette. Final art replaces these components without changing briefing logic.
        MX50ChestVisual->SetStaticMesh(CubeFinder.Object);
        MX50FirstPersonVisual->SetStaticMesh(CubeFinder.Object);
    }
}

void ATU_ArmedOperatorCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (MX50ChestVisual && GetMesh())
    {
        if (GetMesh()->DoesSocketExist(MX50ChestSocket))
        {
            MX50ChestVisual->AttachToComponent(
                GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                MX50ChestSocket);
            MX50ChestVisual->SetRelativeScale3D(MX50ChestFallbackTransform.GetScale3D());
        }
        else
        {
            MX50ChestVisual->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
            MX50ChestVisual->SetRelativeTransform(MX50ChestFallbackTransform);
        }
    }

    if (MX50FirstPersonVisual)
    {
        MX50FirstPersonVisual->SetRelativeTransform(MX50RaisedTransform);
        MX50FirstPersonVisual->SetHiddenInGame(true);
    }

    if (HasAuthority())
    {
        SpawnDefaultWeapon();
        SpawnDefaultMelee();
        if (UTUHealthComponent* Health = FindComponentByClass<UTUHealthComponent>())
        {
            Health->OnDeath.AddDynamic(this, &ATU_ArmedOperatorCharacter::HandleCombatDeath);
        }
    }
}

void ATU_ArmedOperatorCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(MeleeHolsterTimerHandle);
    }

    CloseArmory();
    CloseBriefing();
    DestroyCurrentMelee();
    DestroyLoadoutWeapons();
    Super::EndPlay(EndPlayReason);
}

void ATU_ArmedOperatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::StartWeaponFire);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ATU_ArmedOperatorCharacter::StopWeaponFire);
    PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::ReloadWeapon);
    PlayerInputComponent->BindAction(TEXT("EmergencyReload"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::EmergencyReloadWeapon);
    PlayerInputComponent->BindAction(TEXT("InspectWeapon"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::InspectWeapon);
    PlayerInputComponent->BindAction(TEXT("CycleAction"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::CycleWeaponAction);
    PlayerInputComponent->BindAction(TEXT("CycleFireMode"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::CycleWeaponFireMode);
    PlayerInputComponent->BindAction(TEXT("EquipPrimary"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::EquipPrimaryInput);
    PlayerInputComponent->BindAction(TEXT("EquipSecondary"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::EquipSecondaryInput);
    PlayerInputComponent->BindAction(TEXT("ToggleMelee"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::ToggleMelee);
    PlayerInputComponent->BindAction(TEXT("CycleMelee"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::CycleMeleeInput);
    PlayerInputComponent->BindAction(TEXT("ToggleArmory"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::ToggleArmoryInput);

    // ADS is bound once by the base character and shared with capture/weapon intent.
}

void ATU_ArmedOperatorCharacter::Interact()
{
    if (bCombatDisabled || IsCommandCenterUIOpen() || !GetWorld() || !FirstPersonCamera)
    {
        return;
    }

    if (!HasAuthority())
    {
        ServerInteract();
        return;
    }
    const FVector Start = GetPawnViewLocation();
    const FVector End = Start + GetBaseAimRotation().Vector() * CommandCenterInteractRangeCm;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(CommandCenterInteract), false, this);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return;
    }

    if (ATU_CommandCenterStation* Station = Cast<ATU_CommandCenterStation>(Hit.GetActor()))
    {
        Station->UseStation(this);
    }
    else if (ATU_InteractableBase* Interactable = Cast<ATU_InteractableBase>(Hit.GetActor()))
    {
        Interactable->Interact(this);
    }
    else if (ATUWorldItem* Item = Cast<ATUWorldItem>(Hit.GetActor()))
    {
        Item->TryPickup(CurrentWeapon);
    }
}

ATU_WeaponBase* ATU_ArmedOperatorCharacter::SpawnWeaponClass(TSubclassOf<ATU_WeaponBase> WeaponClass, bool bVisible)
{
    if (!HasAuthority() || !WeaponClass || !GetWorld())
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATU_WeaponBase* Spawned = GetWorld()->SpawnActor<ATU_WeaponBase>(WeaponClass, FTransform::Identity, SpawnParams);
    if (!Spawned)
    {
        return nullptr;
    }

    if (GetWorldWeaponAnchor())
    {
        Spawned->AttachToComponent(
            GetWorldWeaponAnchor(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    }

    Spawned->SetActorHiddenInGame(!bVisible);
    Spawned->SetAiming(bVisible && bIsADS);
    return Spawned;
}

bool ATU_ArmedOperatorCharacter::EnsureWeaponSlotSpawned(ETUOperatorWeaponSlot Slot)
{
    TObjectPtr<ATU_WeaponBase>& SlotWeapon = Slot == ETUOperatorWeaponSlot::Primary ? PrimaryWeapon : SecondaryWeapon;
    if (IsValid(SlotWeapon))
    {
        return true;
    }
    if (!HasAuthority() || (bInventoryHydrated && !bApplyingInventory)) return false;

    TSubclassOf<ATU_WeaponBase> SpawnClass = nullptr;
    if (OperatorLoadout)
    {
        SpawnClass = Slot == ETUOperatorWeaponSlot::Primary
            ? OperatorLoadout->GetSelectedPrimaryClass()
            : OperatorLoadout->GetSelectedSecondaryClass();
    }

    if (!SpawnClass && Slot == ETUOperatorWeaponSlot::Primary)
    {
        SpawnClass = DefaultWeaponClass;
    }

    const bool bShouldBeVisible = Slot == ActiveWeaponSlot && !bMeleeEquipped && !bMX50Raised;
    SlotWeapon = SpawnWeaponClass(SpawnClass, bShouldBeVisible);
    return IsValid(SlotWeapon);
}

bool ATU_ArmedOperatorCharacter::SpawnDefaultWeapon()
{
    if (!HasAuthority()) return false;
    const bool bPrimaryReady = EnsureWeaponSlotSpawned(ETUOperatorWeaponSlot::Primary);
    const bool bSecondaryReady = EnsureWeaponSlotSpawned(ETUOperatorWeaponSlot::Secondary);

    if (!bPrimaryReady && !bSecondaryReady)
    {
        CurrentWeapon = nullptr;
        return false;
    }

    if (bPrimaryReady)
    {
        ActiveWeaponSlot = ETUOperatorWeaponSlot::Primary;
        CurrentWeapon = PrimaryWeapon;
    }
    else
    {
        ActiveWeaponSlot = ETUOperatorWeaponSlot::Secondary;
        CurrentWeapon = SecondaryWeapon;
    }

    if (PrimaryWeapon)
    {
        PrimaryWeapon->SetActorHiddenInGame(CurrentWeapon != PrimaryWeapon || bMeleeEquipped || bMX50Raised);
    }
    if (SecondaryWeapon)
    {
        SecondaryWeapon->SetActorHiddenInGame(CurrentWeapon != SecondaryWeapon || bMeleeEquipped || bMX50Raised);
    }
    if (CurrentWeapon)
    {
        CurrentWeapon->SetAiming(!bMX50Raised && bIsADS);
    }
    OnRep_EquippedWeapons();
    return true;
}

bool ATU_ArmedOperatorCharacter::EquipWeaponSlot(ETUOperatorWeaponSlot Slot)
{
    if (bCombatDisabled) return false;
    if (!HasAuthority()) { ServerEquipWeapon(Slot); return false; }
    if (Slot != ETUOperatorWeaponSlot::Primary && Slot != ETUOperatorWeaponSlot::Secondary) return false;
    if (bMeleeEquipped || bMeleeHolstering || IsCommandCenterUIOpen())
    {
        return false;
    }

    if (!EnsureWeaponSlotSpawned(Slot))
    {
        return false;
    }

    ATU_WeaponBase* Target = Slot == ETUOperatorWeaponSlot::Primary ? PrimaryWeapon : SecondaryWeapon;
    if (!Target)
    {
        return false;
    }

    if (CurrentWeapon == Target)
    {
        ActiveWeaponSlot = Slot;
        return true;
    }

    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
        CurrentWeapon->InterruptWeaponAction();
        CurrentWeapon->SetAiming(false);
        CurrentWeapon->SetActorHiddenInGame(true);
    }

    ActiveWeaponSlot = Slot;
    CurrentWeapon = Target;
    CurrentWeapon->SetActorHiddenInGame(false);
    CurrentWeapon->SetAiming(bIsADS);
    OnRep_EquippedWeapons();
    ForceNetUpdate();
    return true;
}

bool ATU_ArmedOperatorCharacter::ReplaceWeaponSlot(ETUOperatorWeaponSlot Slot)
{
    TObjectPtr<ATU_WeaponBase>& SlotWeapon = Slot == ETUOperatorWeaponSlot::Primary ? PrimaryWeapon : SecondaryWeapon;
    const bool bWasActive = CurrentWeapon == SlotWeapon || ActiveWeaponSlot == Slot;

    if (IsValid(SlotWeapon))
    {
        SlotWeapon->StopFire();
        SlotWeapon->InterruptWeaponAction();
        SlotWeapon->Destroy();
        SlotWeapon = nullptr;
    }

    if (bWasActive)
    {
        CurrentWeapon = nullptr;
    }

    if (!EnsureWeaponSlotSpawned(Slot))
    {
        return false;
    }

    if (bWasActive)
    {
        ActiveWeaponSlot = Slot;
        CurrentWeapon = SlotWeapon;
        CurrentWeapon->SetActorHiddenInGame(bMeleeEquipped || bMX50Raised);
        CurrentWeapon->SetAiming(!bMeleeEquipped && !bMX50Raised && bIsADS);
    }
    else if (SlotWeapon)
    {
        SlotWeapon->SetActorHiddenInGame(true);
        SlotWeapon->SetAiming(false);
    }

    return true;
}

bool ATU_ArmedOperatorCharacter::SelectPrimaryById(FName ItemId)
{
    if (!HasAuthority() || bCombatDisabled || (bInventoryHydrated && !bApplyingInventory) || bMeleeEquipped || bMeleeHolstering || !OperatorLoadout)
    {
        return false;
    }
    if (OperatorLoadout->GetSelectedPrimaryId() == ItemId)
    {
        return true;
    }
    if (!OperatorLoadout->SelectPrimaryById(ItemId))
    {
        return false;
    }
    return ReplaceWeaponSlot(ETUOperatorWeaponSlot::Primary);
}

bool ATU_ArmedOperatorCharacter::SelectSecondaryById(FName ItemId)
{
    if (!HasAuthority() || bCombatDisabled || (bInventoryHydrated && !bApplyingInventory) || bMeleeEquipped || bMeleeHolstering || !OperatorLoadout)
    {
        return false;
    }
    if (OperatorLoadout->GetSelectedSecondaryId() == ItemId)
    {
        return true;
    }
    if (!OperatorLoadout->SelectSecondaryById(ItemId))
    {
        return false;
    }
    return ReplaceWeaponSlot(ETUOperatorWeaponSlot::Secondary);
}

bool ATU_ArmedOperatorCharacter::SelectEquipmentById(FName ItemId)
{
    if (!HasAuthority() || bCombatDisabled || (bInventoryHydrated && !bApplyingInventory) || bMeleeEquipped || bMeleeHolstering || !OperatorLoadout)
    {
        return false;
    }
    return OperatorLoadout->SelectEquipmentById(ItemId);
}

float ATU_ArmedOperatorCharacter::GetSelectedLoadoutWeightKg() const
{
    const float NonMeleeWeight = OperatorLoadout ? OperatorLoadout->GetSelectedNonMeleeWeightKg() : 0.0f;
    return NonMeleeWeight + GetSelectedMeleeWeightKg();
}

void ATU_ArmedOperatorCharacter::DestroyLoadoutWeapons()
{
    if (IsValid(PrimaryWeapon))
    {
        PrimaryWeapon->Destroy();
        PrimaryWeapon = nullptr;
    }
    if (IsValid(SecondaryWeapon))
    {
        SecondaryWeapon->Destroy();
        SecondaryWeapon = nullptr;
    }
    CurrentWeapon = nullptr;
}

FName ATU_ArmedOperatorCharacter::GetSelectedMeleeId() const
{
    return MeleeLoadout ? MeleeLoadout->GetSelectedItemId() : NAME_None;
}

float ATU_ArmedOperatorCharacter::GetSelectedMeleeWeightKg() const
{
    return MeleeLoadout ? MeleeLoadout->GetSelectedWeightKg() : 0.0f;
}

bool ATU_ArmedOperatorCharacter::SpawnDefaultMelee()
{
    if (IsValid(CurrentMelee))
    {
        return true;
    }
    if (!GetWorld())
    {
        return false;
    }

    TSubclassOf<ATU_OTFKnife> SpawnClass = DefaultMeleeClass;
    CurrentMeleeSocket = FirstPersonMeleeSocket;

    FTUMeleeEquipmentEntry SelectedEntry;
    if (MeleeLoadout && MeleeLoadout->GetSelectedItem(SelectedEntry))
    {
        SpawnClass = SelectedEntry.MeleeClass;
        if (!SelectedEntry.EquipSocket.IsNone())
        {
            CurrentMeleeSocket = SelectedEntry.EquipSocket;
        }
    }

    if (!SpawnClass)
    {
        return false;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATU_OTFKnife* Spawned = GetWorld()->SpawnActor<ATU_OTFKnife>(SpawnClass, FTransform::Identity, SpawnParams);
    if (!Spawned)
    {
        return false;
    }

    CurrentMelee = Spawned;
    if (GetWorldWeaponAnchor())
    {
        Spawned->AttachToComponent(
            GetWorldWeaponAnchor(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            CurrentMeleeSocket);
    }
    Spawned->SetActorHiddenInGame(true);
    return true;
}

bool ATU_ArmedOperatorCharacter::SelectMeleeById(FName ItemId)
{
    if (bMeleeEquipped || bMeleeHolstering || !MeleeLoadout)
    {
        return false;
    }
    if (MeleeLoadout->GetSelectedItemId() == ItemId)
    {
        return true;
    }
    if (!MeleeLoadout->SelectItemById(ItemId))
    {
        return false;
    }

    DestroyCurrentMelee();
    return SpawnDefaultMelee();
}

bool ATU_ArmedOperatorCharacter::CycleMeleeSelection(int32 Direction)
{
    if (bMeleeEquipped || bMeleeHolstering || IsCommandCenterUIOpen() || !MeleeLoadout)
    {
        return false;
    }
    if (!MeleeLoadout->CycleSelection(Direction))
    {
        return false;
    }

    DestroyCurrentMelee();
    return SpawnDefaultMelee();
}

bool ATU_ArmedOperatorCharacter::DrawMelee()
{
    if (bMeleeHolstering || IsCommandCenterUIOpen())
    {
        return false;
    }
    if (!IsValid(CurrentMelee) && !SpawnDefaultMelee())
    {
        return false;
    }
    if (bMeleeEquipped)
    {
        return true;
    }

    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
        CurrentWeapon->InterruptWeaponAction();
        CurrentWeapon->SetAiming(false);
        CurrentWeapon->SetActorHiddenInGame(true);
    }

    if (!CurrentMelee->EquipTo(FirstPersonArmsMesh, CurrentMeleeSocket))
    {
        if (CurrentWeapon)
        {
            CurrentWeapon->SetActorHiddenInGame(false);
        }
        return false;
    }

    bMeleeEquipped = true;
    bMeleeHolstering = false;
    return true;
}

bool ATU_ArmedOperatorCharacter::HolsterMelee()
{
    if (!bMeleeEquipped || bMeleeHolstering || !IsValid(CurrentMelee))
    {
        return false;
    }

    bMeleeHolstering = true;
    CurrentMelee->RetractBlade();

    const float Delay = CurrentMelee->GetRetractionDurationSeconds();
    if (!GetWorld() || Delay <= KINDA_SMALL_NUMBER)
    {
        FinishMeleeHolster();
        return true;
    }

    GetWorld()->GetTimerManager().SetTimer(
        MeleeHolsterTimerHandle,
        this,
        &ATU_ArmedOperatorCharacter::FinishMeleeHolster,
        Delay,
        false);
    return true;
}

bool ATU_ArmedOperatorCharacter::OpenArmory()
{
    return OpenArmoryView(ETUArmoryViewMode::Full);
}

bool ATU_ArmedOperatorCharacter::OpenArmoryView(ETUArmoryViewMode ViewMode)
{
    if (bMeleeEquipped || bMeleeHolstering || !ArmoryWidgetClass)
    {
        return false;
    }

    if (IsBriefingOpen())
    {
        CloseBriefing();
    }

    if (IsArmoryOpen())
    {
        ArmoryWidget->SetViewMode(ViewMode);
        return true;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController())
    {
        return false;
    }

    StopADS();
    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
        CurrentWeapon->InterruptWeaponAction();
        CurrentWeapon->SetAiming(false);
    }

    ArmoryWidget = CreateWidget<UTUArmoryWidget>(PC, ArmoryWidgetClass);
    if (!ArmoryWidget)
    {
        return false;
    }

    ArmoryWidget->SetViewMode(ViewMode);
    ArmoryWidget->SetOperator(this);
    ArmoryWidget->AddToPlayerScreen(250);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;
    return true;
}

void ATU_ArmedOperatorCharacter::CloseArmory()
{
    if (IsValid(ArmoryWidget))
    {
        ArmoryWidget->RemoveFromParent();
        ArmoryWidget = nullptr;
    }

    if (!IsBriefingOpen())
    {
        RestoreGameInputMode();
    }
}

void ATU_ArmedOperatorCharacter::ToggleArmory()
{
    if (IsArmoryOpen())
    {
        CloseArmory();
    }
    else
    {
        OpenArmory();
    }
}

bool ATU_ArmedOperatorCharacter::OpenBriefing(FName MissionId, const FText& MissionTitle)
{
    if (bMeleeEquipped || bMeleeHolstering || !BriefingWidgetClass)
    {
        return false;
    }

    if (IsArmoryOpen())
    {
        CloseArmory();
    }

    if (IsBriefingOpen())
    {
        SetMX50Raised(true);
        BriefingWidget->Configure(this, MissionId, MissionTitle);
        return true;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController())
    {
        return false;
    }

    StopADS();
    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
        CurrentWeapon->InterruptWeaponAction();
        CurrentWeapon->SetAiming(false);
    }

    BriefingWidget = CreateWidget<UTUBriefingWidget>(PC, BriefingWidgetClass);
    if (!BriefingWidget)
    {
        return false;
    }

    SetMX50Raised(true);
    BriefingWidget->Configure(this, MissionId, MissionTitle);
    BriefingWidget->AddToPlayerScreen(260);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;
    return true;
}

void ATU_ArmedOperatorCharacter::CloseBriefing()
{
    if (IsValid(BriefingWidget))
    {
        BriefingWidget->RemoveFromParent();
        BriefingWidget = nullptr;
    }

    SetMX50Raised(false);

    if (!IsArmoryOpen())
    {
        RestoreGameInputMode();
    }
}

void ATU_ArmedOperatorCharacter::SetMX50Raised(bool bRaised)
{
    if (bMX50Raised == bRaised)
    {
        return;
    }

    bMX50Raised = bRaised;

    if (MX50FirstPersonVisual)
    {
        MX50FirstPersonVisual->SetHiddenInGame(!bRaised);
    }

    if (MX50ChestVisual)
    {
        // Until final third-person hand animation exists, remove it from the chest while the local operator is using it.
        MX50ChestVisual->SetHiddenInGame(bRaised);
    }

    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
        CurrentWeapon->InterruptWeaponAction();
        CurrentWeapon->SetAiming(false);
        CurrentWeapon->SetActorHiddenInGame(bRaised || bMeleeEquipped);
    }

    BP_OnMX50RaisedChanged(bRaised);
}

void ATU_ArmedOperatorCharacter::RestoreGameInputMode()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC && PC->IsLocalController())
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

void ATU_ArmedOperatorCharacter::StartWeaponFire()
{
    if (bCombatDisabled) return;
    if (IsCommandCenterUIOpen())
    {
        return;
    }
    if (bMeleeEquipped)
    {
        if (!bMeleeHolstering && CurrentMelee)
        {
            CurrentMelee->PerformMeleeAttack();
        }
        return;
    }
    if (CurrentWeapon)
    {
        CurrentWeapon->StartFire();
    }
}

void ATU_ArmedOperatorCharacter::StopWeaponFire()
{
    if (bMeleeEquipped || IsCommandCenterUIOpen())
    {
        return;
    }
    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();

    }
}

bool ATU_ArmedOperatorCharacter::TryReloadFromGameplayInput(bool bEmergency)
{
    if (bCombatDisabled || bMeleeEquipped || IsCommandCenterUIOpen() || !CurrentWeapon) return false;
    if (IsWeaponClearanceBlocked())
    {
        if (APlayerController* PC=Cast<APlayerController>(GetController())) PC->ClientMessage(TEXT("Need more room to reload."));
        return false;
    }
    if (bEmergency)
        return CurrentWeapon->RequestReload(FGuid::NewGuid(), CurrentWeapon->GetActionState().Revision, ETUReloadPolicy::Drop);
    const int32 BeforeRevision=CurrentWeapon->GetActionState().Revision;
    CurrentWeapon->StartReload();
    return CurrentWeapon->GetActionState().bActive || CurrentWeapon->GetActionState().Revision!=BeforeRevision;
}
void ATU_ArmedOperatorCharacter::ReloadWeapon()
{
    TryReloadFromGameplayInput(false);
}

void ATU_ArmedOperatorCharacter::CycleWeaponFireMode()
{
    if (!bMeleeEquipped && !IsCommandCenterUIOpen() && CurrentWeapon)
    {
        CurrentWeapon->CycleFireMode();
    }
}

void ATU_ArmedOperatorCharacter::StartWeaponADS()
{
    StartADS();
}

void ATU_ArmedOperatorCharacter::StopWeaponADS()
{
    StopADS();
}

void ATU_ArmedOperatorCharacter::EquipPrimaryInput()
{
    EquipWeaponSlot(ETUOperatorWeaponSlot::Primary);
}

void ATU_ArmedOperatorCharacter::EquipSecondaryInput()
{
    EquipWeaponSlot(ETUOperatorWeaponSlot::Secondary);
}

void ATU_ArmedOperatorCharacter::ToggleMelee()
{
    if (IsCommandCenterUIOpen())
    {
        return;
    }
    if (bMeleeEquipped)
    {
        HolsterMelee();
    }
    else
    {
        DrawMelee();
    }
}

void ATU_ArmedOperatorCharacter::CycleMeleeInput()
{
    CycleMeleeSelection(1);
}

void ATU_ArmedOperatorCharacter::ToggleArmoryInput()
{
    if (bAllowPortableArmoryDebug)
    {
        ToggleArmory();
    }
}

void ATU_ArmedOperatorCharacter::FinishMeleeHolster()
{
    if (CurrentMelee)
    {
        CurrentMelee->SetActorHiddenInGame(true);
    }

    bMeleeEquipped = false;
    bMeleeHolstering = false;

    if (CurrentWeapon)
    {
        CurrentWeapon->SetActorHiddenInGame(bMX50Raised);
        CurrentWeapon->SetAiming(!bMX50Raised && bIsADS);
    }
}

void ATU_ArmedOperatorCharacter::DestroyCurrentMelee()
{
    if (IsValid(CurrentMelee))
    {
        CurrentMelee->Destroy();
        CurrentMelee = nullptr;
    }
}

void ATU_ArmedOperatorCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATU_ArmedOperatorCharacter, PrimaryWeapon);
    DOREPLIFETIME(ATU_ArmedOperatorCharacter, SecondaryWeapon);
    DOREPLIFETIME(ATU_ArmedOperatorCharacter, CurrentWeapon);
    DOREPLIFETIME(ATU_ArmedOperatorCharacter, ActiveWeaponSlot);
    DOREPLIFETIME(ATU_ArmedOperatorCharacter, bCombatDisabled);
}

void ATU_ArmedOperatorCharacter::OnRep_EquippedWeapons()
{
    if (PrimaryWeapon) PrimaryWeapon->SetActorHiddenInGame(PrimaryWeapon != CurrentWeapon || bMeleeEquipped || bMX50Raised);
    if (SecondaryWeapon) SecondaryWeapon->SetActorHiddenInGame(SecondaryWeapon != CurrentWeapon || bMeleeEquipped || bMX50Raised);
    if (GetWeaponPresentation()) GetWeaponPresentation()->InitializeForWeapon(CurrentWeapon);
}

void ATU_ArmedOperatorCharacter::ServerInteract_Implementation()
{
    Interact(); // The server recomputes its own ray, distance and eligibility.
}

void ATU_ArmedOperatorCharacter::ServerEquipWeapon_Implementation(ETUOperatorWeaponSlot RequestedSlot)
{
    EquipWeaponSlot(RequestedSlot);
}

void ATU_ArmedOperatorCharacter::OnRep_CombatDisabled()
{
    if (!bCombatDisabled) return;
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }
    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
    }
}

void ATU_ArmedOperatorCharacter::DisableCombatForOutcome()
{
    if (!HasAuthority()) return;
    bCombatDisabled = true;
    OnRep_CombatDisabled();
    ForceNetUpdate();
}

void ATU_ArmedOperatorCharacter::HandleCombatDeath(AActor* DeadActor)
{
    if (DeadActor != this || !HasAuthority() || bCombatDisabled) return;
    DisableCombatForOutcome();
    if (ATU_GameMode* Raid = GetWorld() ? GetWorld()->GetAuthGameMode<ATU_GameMode>() : nullptr)
    {
        Raid->SetParticipantLedger(this, ExportItemLedger());
        Raid->ResolvePlayerOutcome(this, ETURaidPlayerOutcome::Dead);
    }
}

void ATU_ArmedOperatorCharacter::EmergencyReloadWeapon()
{
    if (bCombatDisabled || bMeleeEquipped || IsCommandCenterUIOpen() || !CurrentWeapon) return;
    if (IsWeaponClearanceBlocked())
    {
        if (APlayerController* PC=Cast<APlayerController>(GetController())) PC->ClientMessage(TEXT("Need more room to reload."));
        return;
    }
    CurrentWeapon->RequestReload(FGuid::NewGuid(), CurrentWeapon->GetActionState().Revision, ETUReloadPolicy::Drop);
}

void ATU_ArmedOperatorCharacter::InspectWeapon()
{
    if (bCombatDisabled || !CurrentWeapon) return;
    const FMagazineState State = CurrentWeapon->Inspect();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->ClientMessage(FString::Printf(TEXT("Magazine: %d / %d | Chamber: %s"), State.RoundsInMagazine,
            State.Capacity, State.bRoundChambered ? TEXT("loaded") : TEXT("empty")));
    }
}

void ATU_ArmedOperatorCharacter::CycleWeaponAction()
{
    if (!bCombatDisabled && !bMeleeEquipped && !IsCommandCenterUIOpen() && CurrentWeapon) CurrentWeapon->CycleAction();
}

FTUItemLedger ATU_ArmedOperatorCharacter::ExportItemLedger() const
{
    FTUItemLedger Combined = UnarmedInventory;
    const ATU_WeaponBase* Actors[] = { PrimaryWeapon.Get(), SecondaryWeapon.Get() };
    const FName Slots[] = { TEXT("Primary"), TEXT("Secondary") };
    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (!IsValid(Actors[Index])) continue;
        FTUItemLedger Part = Actors[Index]->ExportItemLedger();
        for (FWeaponInstanceState& Weapon : Part.Weapons) Weapon.LoadoutSlot = Slots[Index];
        Combined.Weapons.Append(Part.Weapons);
        Combined.Magazines.Append(Part.Magazines);
        Combined.Items.Append(Part.Items);
        Combined.LooseCartridges.Append(Part.LooseCartridges);
        Combined.WeaponActions.Append(Part.WeaponActions);
        Combined.Revision = FMath::Max(Combined.Revision, Part.Revision);
    }
    return Combined;
}

bool ATU_ArmedOperatorCharacter::ImportItemLedger(const FTUItemLedger& Ledger)
{
    if (!HasAuthority() || Ledger.Weapons.Num() > 2) return false;
    TSet<FGuid> Seen;
    for (const FWeaponInstanceState& Weapon : Ledger.Weapons)
    {
        if (!Weapon.InstanceId.IsValid() || Seen.Contains(Weapon.InstanceId)) return false;
        Seen.Add(Weapon.InstanceId);
    }
    for (const FTUMagazineInstance& Magazine : Ledger.Magazines)
    {
        if (!Magazine.InstanceId.IsValid() || Seen.Contains(Magazine.InstanceId)) return false;
        Seen.Add(Magazine.InstanceId);
    }
    for (const FTUItemInstance& Item : Ledger.Items)
    {
        if (!Item.InstanceId.IsValid() || Seen.Contains(Item.InstanceId)) return false;
        Seen.Add(Item.InstanceId);
    }
    TGuardValue<bool> Applying(bApplyingInventory, true);
    FTUItemLedger Parts[2];
    bool Used[2] = {false, false};
    for (const FWeaponInstanceState& Weapon : Ledger.Weapons)
    {
        const int32 Index = Weapon.LoadoutSlot == TEXT("Secondary") ? 1 : 0;
        if (!Weapon.LoadoutSlot.IsNone() && Weapon.LoadoutSlot != TEXT("Primary") && Weapon.LoadoutSlot != TEXT("Secondary")) return false;
        if (Used[Index]) return false;
        Used[Index] = true;
        Parts[Index].Weapons.Add(Weapon);
        Parts[Index].Revision = Ledger.Revision;
        for (const FTUMagazineInstance& Magazine : Ledger.Magazines)
            if (Magazine.WeaponId == Weapon.InstanceId) Parts[Index].Magazines.Add(Magazine);
        for (const FTUWeaponActionState& Action : Ledger.WeaponActions)
            if (Action.WeaponId == Weapon.InstanceId) Parts[Index].WeaponActions.Add(Action);
    }
    // Generic carried goods and loose rounds have exactly one container, even when both guns exist.
    const int32 CargoIndex = Used[0] ? 0 : 1;
    if (Used[0] || Used[1])
    {
        if (Parts[0].Magazines.Num() + Parts[1].Magazines.Num() != Ledger.Magazines.Num() ||
            Parts[0].WeaponActions.Num() + Parts[1].WeaponActions.Num() != Ledger.WeaponActions.Num()) return false;
    }
    if (Used[CargoIndex])
    {
        Parts[CargoIndex].Items = Ledger.Items;
        Parts[CargoIndex].LooseCartridges = Ledger.LooseCartridges;
    }
    ATU_WeaponBase* Previous[2] = { PrimaryWeapon.Get(), SecondaryWeapon.Get() };
    ATU_WeaponBase* Candidates[2] = { nullptr, nullptr };
    bool Created[2] = { false, false };
    FName SelectedIds[2];
    auto DiscardCandidates = [&]()
    {
        for (int32 Index = 0; Index < 2; ++Index)
            if (Created[Index] && Candidates[Index]) Candidates[Index]->Destroy();
    };
    // Resolve the durable definition to its actual runtime class before importing.
    // A saved AK or pistol must never inherit the current slot actor's ballistics.
    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (!Used[Index]) continue;
        const FName Definition = Parts[Index].Weapons[0].DefinitionId;
        TSubclassOf<ATU_WeaponBase> ResolvedClass;
        if (OperatorLoadout)
        {
            const auto Entries = Index == 0 ? OperatorLoadout->GetPrimaryItems() : OperatorLoadout->GetSecondaryItems();
            for (const FTUOperatorWeaponEntry& Entry : Entries)
                if (Entry.WeaponClass && Entry.WeaponClass.GetDefaultObject()->GetWeaponDefinition().WeaponId == Definition)
                { ResolvedClass = Entry.WeaponClass; SelectedIds[Index] = Entry.ItemId; break; }
        }
        if (!ResolvedClass && Previous[Index] && Previous[Index]->GetWeaponDefinition().WeaponId == Definition)
            ResolvedClass = Previous[Index]->GetClass();
        if (!ResolvedClass) { DiscardCandidates(); return false; }
        Candidates[Index] = Previous[Index] && Previous[Index]->GetClass() == ResolvedClass.Get()
            ? Previous[Index] : SpawnWeaponClass(ResolvedClass, false);
        Created[Index] = Candidates[Index] && Candidates[Index] != Previous[Index];
        if (!Candidates[Index] || !Candidates[Index]->ValidateItemLedger(Parts[Index]))
        { DiscardCandidates(); return false; }
    }
    // Validation of every partition precedes mutation of any existing actor.
    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (Used[Index]) Candidates[Index]->ImportItemLedger(Parts[Index]);
        if (Previous[Index] && Previous[Index] != Candidates[Index]) Previous[Index]->Destroy();
        if (OperatorLoadout && !SelectedIds[Index].IsNone())
        {
            if (Index == 0) OperatorLoadout->SelectPrimaryById(SelectedIds[Index]);
            else OperatorLoadout->SelectSecondaryById(SelectedIds[Index]);
        }
    }
    PrimaryWeapon = Candidates[0]; SecondaryWeapon = Candidates[1];
    UnarmedInventory = FTUItemLedger();
    if (!Used[0] && !Used[1]) UnarmedInventory = Ledger;
    bInventoryHydrated = true;
    CurrentWeapon = ActiveWeaponSlot == ETUOperatorWeaponSlot::Primary ? PrimaryWeapon : SecondaryWeapon;
    if (!CurrentWeapon)
    {
        CurrentWeapon = PrimaryWeapon ? PrimaryWeapon : SecondaryWeapon;
        ActiveWeaponSlot = PrimaryWeapon ? ETUOperatorWeaponSlot::Primary : ETUOperatorWeaponSlot::Secondary;
    }
    OnRep_EquippedWeapons();
    ForceNetUpdate();
    return true;
}
