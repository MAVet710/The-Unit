#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_RangeTarget.generated.h"

class UStaticMeshComponent;

/** Reusable live-fire target for the Command Center range. Uses the normal Unreal damage path. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_RangeTarget : public AActor
{
    GENERATED_BODY()

public:
    ATU_RangeTarget();

    virtual void BeginPlay() override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category="Range Target")
    void ResetTarget();

    UFUNCTION(BlueprintPure, Category="Range Target")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category="Range Target")
    int32 GetHitCount() const { return HitCount; }

    UFUNCTION(BlueprintPure, Category="Range Target")
    int32 GetResetCount() const { return ResetCount; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Range Target")
    TObjectPtr<UStaticMeshComponent> TargetMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range Target", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range Target", meta=(ClampMin="0.05"))
    float ResetDelaySeconds = 0.8f;

private:
    float CurrentHealth = 100.0f;
    int32 HitCount = 0;
    int32 ResetCount = 0;
    FTimerHandle ResetTimerHandle;
};
