#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TU_OperatorCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UTUWeaponPresentationComponent;
class UAnimSequence;

/**
 * Base controllable operator pawn.
 * Owns first-person movement/input skeleton that can be extended in Blueprints.
 */
UCLASS()
class THEUNIT_API ATU_OperatorCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ATU_OperatorCharacter();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    USceneComponent* GetWorldWeaponAnchor() const { return WorldWeaponAnchor; }
    UTUWeaponPresentationComponent* GetWeaponPresentation() const { return WeaponPresentation; }
    USkeletalMeshComponent* GetOwnerBodyMesh() const { return FirstPersonArmsMesh; }
    FTransform GetHandlingEyeWorld() const;
    float GetHandlingADSAlpha() const { return HandlingADSAlpha; }
    bool IsWeaponClearanceBlocked() const { return bWeaponClearanceBlocked; }
    void ApplyHandlingCapturePose(bool bADS, bool bCrouched, float Lean, float PitchDegrees);
    virtual bool CanAimWeapon() const { return true; }
    void SuspendInputForMenu();
    UFUNCTION(BlueprintPure) bool IsWeaponRaised() const { return ReadyPosture == 0 && !bIsSprinting && !bWeaponClearanceBlocked && HandlingManipulationAlpha<.02f; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> WorldWeaponAnchor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTUWeaponPresentationComponent> WeaponPresentation;
    UPROPERTY() TObjectPtr<UAnimSequence> PrototypeIdle;
    UPROPERTY(Replicated) uint8 ReplicatedPosture = 0;
    UFUNCTION(Server, Reliable) void ServerSetPosture(uint8 Flags);
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Movement|State") uint8 ReadyPosture = 0;
    UFUNCTION(BlueprintCallable) void CycleReadyPosture();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float WalkSpeed = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float SprintSpeed = 550.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float CrouchSpeed = 180.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float ADSMovementMultiplier = 0.7f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State")
    bool bIsSprinting = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State")
    bool bIsADS = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State")
    bool bIsLeaningLeft = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State")
    bool bIsLeaningRight = false;

    UFUNCTION(BlueprintCallable, Category = "Input|Movement")
    void MoveForward(float Value);

    UFUNCTION(BlueprintCallable, Category = "Input|Movement")
    void MoveRight(float Value);

    UFUNCTION(BlueprintCallable, Category = "Input|Look")
    void LookUp(float Value);

    UFUNCTION(BlueprintCallable, Category = "Input|Look")
    void Turn(float Value);

    UFUNCTION(BlueprintCallable, Category = "Input|Movement")
    void StartSprint();

    UFUNCTION(BlueprintCallable, Category = "Input|Movement")
    void StopSprint();

    UFUNCTION(BlueprintCallable, Category = "Input|Movement")
    void StartCrouch();

    UFUNCTION(BlueprintCallable, Category = "Input|Movement")
    void StopCrouch();

    UFUNCTION(BlueprintCallable, Category = "Input|Aim")
    void StartADS();

    UFUNCTION(BlueprintCallable, Category = "Input|Aim")
    void StopADS();

    UFUNCTION(BlueprintCallable, Category = "Input|Lean")
    void StartLeanLeft();

    UFUNCTION(BlueprintCallable, Category = "Input|Lean")
    void StopLeanLeft();

    UFUNCTION(BlueprintCallable, Category = "Input|Lean")
    void StartLeanRight();

    UFUNCTION(BlueprintCallable, Category = "Input|Lean")
    void StopLeanRight();

    UFUNCTION(BlueprintCallable, Category = "Input|Interaction")
    virtual void Interact();

private:
    float HandlingADSAlpha = 0.f;
    float HandlingLean = 0.f;
    float HandlingReadyPitch = 0.f;
    float HandlingManipulationAlpha = 0.f;
    bool bWeaponClearanceBlocked=false;
    FTransform PreviousClearWeaponPose=FTransform::Identity;
    void UpdateMovementSpeed();
    void UpdatePosture();
};
