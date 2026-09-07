#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TU_OperatorCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UTUHealthComponent;

/**
 * Base controllable operator pawn.
 * Owns first-person movement/input skeleton and operator-level reusable capabilities.
 */
UCLASS()
class THEUNIT_API ATU_OperatorCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ATU_OperatorCharacter();

    virtual void PostInitializeComponents() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category = "Operator|Health")
    UTUHealthComponent* GetHealthComponent() const;

    UFUNCTION(BlueprintPure, Category = "Operator|Health")
    bool IsOperatorDead() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTUHealthComponent> HealthComponent;

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

    UFUNCTION()
    void HandleOperatorDeath(AActor* DeadActor);

private:
    UPROPERTY(VisibleAnywhere, Category = "Operator|Health")
    bool bDeathHandled = false;

    void UpdateMovementSpeed();
};
