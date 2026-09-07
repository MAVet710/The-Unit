#include "TU_OperatorCharacter.h"

#include "TU_FPVDrone.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

ATU_OperatorCharacter::ATU_OperatorCharacter()
{
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->bUsePawnControlRotation = true;

    FirstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmsMesh"));
    FirstPersonArmsMesh->SetupAttachment(FirstPersonCamera);
    FirstPersonArmsMesh->SetOnlyOwnerSee(true);
    FirstPersonArmsMesh->bCastDynamicShadow = false;
    FirstPersonArmsMesh->CastShadow = false;

    FPVDroneClass = ATU_FPVDrone::StaticClass();

    bUseControllerRotationYaw = true;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = WalkSpeed;
        Movement->MaxWalkSpeedCrouched = CrouchSpeed;
        Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
    }
}

void ATU_OperatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATU_OperatorCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATU_OperatorCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ATU_OperatorCharacter::LookUp);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ATU_OperatorCharacter::Turn);

    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ATU_OperatorCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ATU_OperatorCharacter::StopSprint);

    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ATU_OperatorCharacter::StartCrouch);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &ATU_OperatorCharacter::StopCrouch);

    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Pressed, this, &ATU_OperatorCharacter::StartADS);
    PlayerInputComponent->BindAction(TEXT("ADS"), IE_Released, this, &ATU_OperatorCharacter::StopADS);

    PlayerInputComponent->BindAction(TEXT("LeanLeft"), IE_Pressed, this, &ATU_OperatorCharacter::StartLeanLeft);
    PlayerInputComponent->BindAction(TEXT("LeanLeft"), IE_Released, this, &ATU_OperatorCharacter::StopLeanLeft);

    PlayerInputComponent->BindAction(TEXT("LeanRight"), IE_Pressed, this, &ATU_OperatorCharacter::StartLeanRight);
    PlayerInputComponent->BindAction(TEXT("LeanRight"), IE_Released, this, &ATU_OperatorCharacter::StopLeanRight);

    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ATU_OperatorCharacter::Interact);
    PlayerInputComponent->BindAction(TEXT("FPVDeploy"), IE_Pressed, this, &ATU_OperatorCharacter::DeployOrEnterFPVDrone);
}

void ATU_OperatorCharacter::MoveForward(float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void ATU_OperatorCharacter::MoveRight(float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void ATU_OperatorCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void ATU_OperatorCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void ATU_OperatorCharacter::StartSprint()
{
    bIsSprinting = true;
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StopSprint()
{
    bIsSprinting = false;
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StartCrouch()
{
    Crouch();
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StopCrouch()
{
    UnCrouch();
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StartADS()
{
    bIsADS = true;
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StopADS()
{
    bIsADS = false;
    UpdateMovementSpeed();
}

void ATU_OperatorCharacter::StartLeanLeft()
{
    bIsLeaningLeft = true;
    bIsLeaningRight = false;
}

void ATU_OperatorCharacter::StopLeanLeft()
{
    bIsLeaningLeft = false;
}

void ATU_OperatorCharacter::StartLeanRight()
{
    bIsLeaningRight = true;
    bIsLeaningLeft = false;
}

void ATU_OperatorCharacter::StopLeanRight()
{
    bIsLeaningRight = false;
}

void ATU_OperatorCharacter::Interact()
{
    // Placeholder: interaction logic implemented in later phases.
}

void ATU_OperatorCharacter::DeployOrEnterFPVDrone()
{
    AController* OperatorController = GetController();
    if (!OperatorController || !GetWorld() || !FPVDroneClass)
    {
        return;
    }

    if (!IsValid(ActiveFPVDrone))
    {
        const FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 80.0f) + FVector(0.0f, 0.0f, 35.0f);
        const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ActiveFPVDrone = GetWorld()->SpawnActor<ATU_FPVDrone>(FPVDroneClass, SpawnLocation, SpawnRotation, SpawnParams);
    }

    if (IsValid(ActiveFPVDrone))
    {
        ActiveFPVDrone->SetReturnPawn(this);
        OperatorController->Possess(ActiveFPVDrone);
    }
}

void ATU_OperatorCharacter::UpdateMovementSpeed()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        float TargetSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;

        if (bIsCrouched)
        {
            TargetSpeed = CrouchSpeed;
        }

        if (bIsADS)
        {
            TargetSpeed *= ADSMovementMultiplier;
        }

        Movement->MaxWalkSpeed = TargetSpeed;
        Movement->MaxWalkSpeedCrouched = CrouchSpeed * ADSMovementMultiplier;
    }
}
