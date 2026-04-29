#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheUnitTypes.h"
#include "TUCalloutManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTUCalloutTriggered, ETUCalloutCategory, Category, FText, Subtitle);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUCalloutManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<ETUCalloutCategory, float> CategoryCooldownSeconds;
	UPROPERTY(BlueprintAssignable) FTUCalloutTriggered OnCalloutTriggered;

	UTUCalloutManagerComponent();
	UFUNCTION(BlueprintCallable) bool TryTriggerCallout(ETUCalloutCategory Category, const FText& Subtitle);

private:
	TMap<ETUCalloutCategory, float> LastTriggerTime;
};
