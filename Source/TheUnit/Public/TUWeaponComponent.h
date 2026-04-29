#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheUnitTypes.h"
#include "TUWeaponComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUWeaponComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FWeaponDefinition WeaponDefinition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FAmmoDefinition AmmoDefinition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMagazineState MagazineState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AmmoReserve = 90;

	UFUNCTION(BlueprintCallable) bool FireSemiAuto();
	UFUNCTION(BlueprintCallable) bool Reload();
};
