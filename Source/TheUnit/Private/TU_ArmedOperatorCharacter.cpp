#include "TU_ArmedOperatorCharacter.h"

#include "TUArmoryWidget.h"
#include "TUBriefingWidget.h"
#include "TUMeleeLoadoutComponent.h"
#include "TU_CommandCenterStation.h"
#include "TU_OTFKnife.h"
#include "TU_TacticalRifle.h"
#include "TU_WeaponBase.h"
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

    SpawnDefaultWeapon();
    SpawnDefaultMelee();
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
    PlayerInputComponent->BindAction(TEXT("CycleFireMode"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::CycleWeaponFireMode);
    PlayerInputComponent->BindAction(TEXT("EquipPrimary"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::EquipPrimaryInput);
    PlayerInputComponent->BindAction(TEXT("EquipSecondary"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::EquipSecondaryInput);
    PlayerInputComponent->BindAction(TEXT("ToggleMelee"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::ToggleMelee);
    PlayerInputComponent->BindAction(TEXT("CycleMelee"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::CycleMeleeInput);
    PlayerInputComponent->BindAction(TEXT("ToggleArmory"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::ToggleArmoryInput);

    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::StartWeaponADS);
    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Released, this, &ATU_ArmedOperatorCharacter::StopWeaponADS);
}

void ATU_ArmedOperatorCharacter::Interact()
{
    if (IsCommandCenterUIOpen() || !GetWorld() || !FirstPersonCamera)
    {
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * CommandCenterInteractRangeCm;

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
}

ATU_WeaponBase* ATU_ArmedOperatorCharacter::SpawnWeaponClass(TSubclassOf<ATU_WeaponBase> WeaponClass, bool bVisible)
{
    if (!WeaponClass || !GetWorld())
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

    if (FirstPersonArmsMesh)
    {
        Spawned->AttachToComponent(
            FirstPersonArmsMesh,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            FirstPersonWeaponSocket);
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
    return true;
}

bool ATU_ArmedOperatorCharacter::EquipWeaponSlot(ETUOperatorWeaponSlot Slot)
{
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
        CurrentWeapon->SetAiming(false);
        CurrentWeapon->SetActorHiddenInGame(true);
    }

    ActiveWeaponSlot = Slot;
    CurrentWeapon = Target;
    CurrentWeapon->SetActorHiddenInGame(false);
    CurrentWeapon->SetAiming(bIsADS);
    return true;
}

bool ATU_ArmedOperatorCharacter::ReplaceWeaponSlot(ETUOperatorWeaponSlot Slot)
{
    TObjectPtr<ATU_WeaponBase>& SlotWeapon = Slot == ETUOperatorWeaponSlot::Primary ? PrimaryWeapon : SecondaryWeapon;
    const bool bWasActive = CurrentWeapon == SlotWeapon || ActiveWeaponSlot == Slot;

    if (IsValid(SlotWeapon))
    {
        SlotWeapon->StopFire();
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
    if (bMeleeEquipped || bMeleeHolstering || !OperatorLoadout)
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
    if (bMeleeEquipped || bMeleeHolstering || !OperatorLoadout)
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
    if (bMeleeEquipped || bMeleeHolstering || !OperatorLoadout)
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
    if (FirstPersonArmsMesh)
    {
        Spawned->AttachToComponent(
            FirstPersonArmsMesh,
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

void ATU_ArmedOperatorCharacter::ReloadWeapon()
{
    if (!bMeleeEquipped && !IsCommandCenterUIOpen() && CurrentWeapon)
    {
        CurrentWeapon->StartReload();
    }
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
    if (!bMeleeEquipped && !IsCommandCenterUIOpen() && CurrentWeapon)
    {
        CurrentWeapon->SetAiming(true);
    }
}

void ATU_ArmedOperatorCharacter::StopWeaponADS()
{
    if (!bMeleeEquipped && !IsCommandCenterUIOpen() && CurrentWeapon)
    {
        CurrentWeapon->SetAiming(false);
    }
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
