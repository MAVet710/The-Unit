#include "TU_ArmedOperatorCharacter.h"

#include "TU_TacticalRifle.h"
#include "TU_WeaponBase.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

ATU_ArmedOperatorCharacter::ATU_ArmedOperatorCharacter()
{
    DefaultWeaponClass = ATU_TacticalRifle::StaticClass();
}

void ATU_ArmedOperatorCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnDefaultWeapon();
}

void ATU_ArmedOperatorCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

    // Base operator already owns the ADS movement state. These additional callbacks keep the weapon spread state in sync.
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

void ATU_ArmedOperatorCharacter::StartWeaponFire()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->StartFire();
    }
}

void ATU_ArmedOperatorCharacter::StopWeaponFire()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
    }
}

void ATU_ArmedOperatorCharacter::ReloadWeapon()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->StartReload();
    }
}

void ATU_ArmedOperatorCharacter::CycleWeaponFireMode()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->CycleFireMode();
    }
}

void ATU_ArmedOperatorCharacter::StartWeaponADS()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->SetAiming(true);
    }
}

void ATU_ArmedOperatorCharacter::StopWeaponADS()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->SetAiming(false);
    }
}
