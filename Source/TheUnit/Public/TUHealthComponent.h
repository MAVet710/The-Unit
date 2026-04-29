#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheUnitTypes.h"
#include "TUHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTUOnDeath, AActor*, DeadActor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTUHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxTotalHealth = 440.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<ETUBodyRegion, float> BodyPartHealth;
	UPROPERTY(BlueprintReadOnly) bool bBleeding = false;
	UPROPERTY(BlueprintReadOnly) bool bLimping = false;
	UPROPERTY(BlueprintReadOnly) bool bHeavyBreathing = false;
	UPROPERTY(BlueprintReadOnly) bool bSuppressed = false;
	UPROPERTY(BlueprintAssignable) FTUOnDeath OnDeath;

	UFUNCTION(BlueprintCallable) void ApplyRegionalDamage(ETUBodyRegion Region, float Damage);
	UFUNCTION(BlueprintPure) float GetTotalHealth() const;
	UFUNCTION(BlueprintPure) bool IsDead() const;
};
