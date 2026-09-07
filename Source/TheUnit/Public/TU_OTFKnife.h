#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_OTFKnife.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ETUOTFBladeState : uint8
{
    Retracted UMETA(DisplayName="Retracted"),
    Deploying UMETA(DisplayName="Deploying"),
    Deployed UMETA(DisplayName="Deployed"),
    Retracting UMETA(DisplayName="Retracting")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTUOTFBladeEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTUOTFMeleeHitEvent, AActor*, HitActor);

/**
 * First-person OTF-style melee item for The Unit.
 * The blade is a separate presentation mesh that translates between authored
 * retracted/deployed transforms. No real internal OTF mechanism is modeled.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_OTFKnife : public AActor
{
    GENERATED_BODY()

public:
    ATU_OTFKnife();

    virtual void Tick(float DeltaSeconds) override;

    /** Attach to a first-person hand/socket, reset retracted, then deploy. */
    UFUNCTION(BlueprintCallable, Category="Melee|Equip")
    bool EquipTo(USkeletalMeshComponent* ParentMesh, FName SocketName);

    UFUNCTION(BlueprintCallable, Category="Melee|Blade")
    void DeployBlade();

    UFUNCTION(BlueprintCallable, Category="Melee|Blade")
    void RetractBlade();

    UFUNCTION(BlueprintCallable, Category="Melee|Blade")
    void ToggleBlade();

    /** Short first-person melee sweep; only valid while fully deployed. */
    UFUNCTION(BlueprintCallable, Category="Melee|Attack")
    bool PerformMeleeAttack();

    UFUNCTION(BlueprintPure, Category="Melee|Blade")
    ETUOTFBladeState GetBladeState() const { return BladeState; }

    UFUNCTION(BlueprintPure, Category="Melee|Blade")
    bool IsBladeDeployed() const { return BladeState == ETUOTFBladeState::Deployed; }

    UFUNCTION(BlueprintPure, Category="Melee|Blade")
    float GetBladeAlpha() const { return BladeAlpha; }

    UFUNCTION(BlueprintPure, Category="Melee|Blade")
    float GetRetractionDurationSeconds() const { return RetractionDurationSeconds; }

    UFUNCTION(BlueprintPure, Category="Melee|Visual")
    UStaticMeshComponent* GetHandleMesh() const { return HandleMesh; }

    UFUNCTION(BlueprintPure, Category="Melee|Visual")
    UStaticMeshComponent* GetBladeMesh() const { return BladeMesh; }

    UPROPERTY(BlueprintAssignable, Category="Melee|Blade")
    FTUOTFBladeEvent OnBladeDeployed;

    UPROPERTY(BlueprintAssignable, Category="Melee|Blade")
    FTUOTFBladeEvent OnBladeRetracted;

    UPROPERTY(BlueprintAssignable, Category="Melee|Attack")
    FTUOTFMeleeHitEvent OnMeleeHit;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Visual")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Visual")
    TObjectPtr<UStaticMeshComponent> HandleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Visual")
    TObjectPtr<UStaticMeshComponent> BladeMesh;

    /** Game-authored blade transform while hidden inside the handle. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Blade")
    FVector RetractedBladeLocation = FVector(-12.5f, 0.0f, 0.0f);

    /** Game-authored blade transform while fully visible. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Blade")
    FVector DeployedBladeLocation = FVector(0.8f, 0.0f, 0.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Blade", meta=(ClampMin="0.01", ClampMax="1.0"))
    float DeploymentDurationSeconds = 0.12f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Blade", meta=(ClampMin="0.01", ClampMax="1.0"))
    float RetractionDurationSeconds = 0.12f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Attack", meta=(ClampMin="0.0"))
    float MeleeDamage = 45.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Attack", meta=(ClampMin="25.0", ClampMax="300.0"))
    float MeleeRangeCm = 165.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee|Attack", meta=(ClampMin="1.0", ClampMax="30.0"))
    float MeleeSweepRadiusCm = 7.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Blade")
    ETUOTFBladeState BladeState = ETUOTFBladeState::Retracted;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Blade")
    float BladeAlpha = 0.0f;

private:
    void BeginBladeMotion(ETUOTFBladeState NewState, float TargetAlpha);
    void SetBladeAlpha(float NewAlpha);
    void CompleteBladeMotion();

    float MotionStartAlpha = 0.0f;
    float MotionTargetAlpha = 0.0f;
    float MotionElapsedSeconds = 0.0f;
};
