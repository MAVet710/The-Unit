#include "TU_ArmedOperatorCharacter.h"

#include "TU_OTFKnife.h"
#include "TU_TacticalRifle.h"
#include "TU_WeaponBase.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ATU_ArmedOperatorCharacter::ATU_ArmedOperatorCharacter()
{
    DefaultWeaponClass = ATU_TacticalRifle::StaticClass();
    DefaultMeleeClass = ATU_OTFKnife::StaticClass();
}

void ATU_ArmedOperatorCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnDefaultWeapon();
    SpawnDefaultMelee();
}

void ATU_ArmedOperatorCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(MeleeHolsterTimerHandle);
    }

    if (IsValid(CurrentMelee))
    {
        CurrentMelee->Destroy();
        CurrentMelee = nullptr;
    }

    if (IsValid(CurrentWeapon))
    {
        CurrentWeapon->Destroy();
        CurrentWeapon = nullptr;
    }

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
    PlayerInputComponent->BindAction(TEXT("ToggleMelee"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::ToggleMelee);

    // Base operator already owns the ADS movement state. These additional callbacks keep the firearm spread state in sync.
    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Pressed, this, &ATU_ArmedOperatorCharacter::StartWeaponADS);
    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Released, this, &ATU_ArmedOperatorCharacter::StopWeaponADS);
}

bool ATU_ArmedOperatorCharacter::SpawnDefaultWeapon()
{
    if (IsValid(CurrentWeapon) || !DefaultWeaponClass || !GetWorld())
    {
        return IsValid(CurrentWeapon);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATU_WeaponBase* Spawned = GetWorld()->SpawnActor<ATU_WeaponBase>(DefaultWeaponClass, FTransform::Identity, SpawnParams);
    if (!Spawned)
    {
        return false;
    }

    CurrentWeapon = Spawned;
    if (FirstPersonArmsMesh)
    {
        Spawned->AttachToComponent(
            FirstPersonArmsMesh,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            FirstPersonWeaponSocket);
    }

    Spawned->SetAiming(bIsADS);
    return true;
}

bool ATU_ArmedOperatorCharacter::SpawnDefaultMelee()
{
    if (IsValid(CurrentMelee) || !DefaultMeleeClass || !GetWorld())
    {
        return IsValid(CurrentMelee);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATU_OTFKnife* Spawned = GetWorld()->SpawnActor<ATU_OTFKnife>(DefaultMeleeClass, FTransform::Identity, SpawnParams);
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
            FirstPersonMeleeSocket);
    }
    Spawned->SetActorHiddenInGame(true);
    return true;
}

bool ATU_ArmedOperatorCharacter::DrawMelee()
{
    if (bMeleeHolstering)
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
        CurrentWeapon->SetActorHiddenInGame(true);
    }

    if (!CurrentMelee->EquipTo(FirstPersonArmsMesh, FirstPersonMeleeSocket))
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

void ATU_ArmedOperatorCharacter::StartWeaponFire()
{
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
    if (bMeleeEquipped)
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
    if (!bMeleeEquipped && CurrentWeapon)
    {
        CurrentWeapon->StartReload();
    }
}

void ATU_ArmedOperatorCharacter::CycleWeaponFireMode()
{
    if (!bMeleeEquipped && CurrentWeapon)
    {
        CurrentWeapon->CycleFireMode();
    }
}

void ATU_ArmedOperatorCharacter::StartWeaponADS()
{
    if (!bMeleeEquipped && CurrentWeapon)
    {
        CurrentWeapon->SetAiming(true);
    }
}

void ATU_ArmedOperatorCharacter::StopWeaponADS()
{
    if (!bMeleeEquipped && CurrentWeapon)
    {
        CurrentWeapon->SetAiming(false);
    }
}

void ATU_ArmedOperatorCharacter::ToggleMelee()
{
    if (bMeleeEquipped)
    {
        HolsterMelee();
    }
    else
    {
        DrawMelee();
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
        CurrentWeapon->SetActorHiddenInGame(false);
        CurrentWeapon->SetAiming(bIsADS);
    }
}
