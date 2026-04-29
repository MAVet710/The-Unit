#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TU_OperatorCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;

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

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMesh;

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
    void UpdateMovementSpeed();
};
