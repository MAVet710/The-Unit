#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheUnitTypes.h"
#include "TUWeaponComponent.generated.h"

/** Internal ammunition mechanics for ATU_WeaponBase; not a standalone weapon API. */
UCLASS(NotBlueprintable)
class THEUNIT_API UTUWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUWeaponComponent();

private:
    friend class ATU_WeaponBase;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    FWeaponDefinition WeaponDefinition;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    FAmmoDefinition AmmoDefinition;

    // Magazine rounds exclude the chamber. Only this component mutates either.
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    FMagazineState MagazineState;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 AmmoReserve = 90;

    bool HasAmmo() const;
    bool ConsumeRound();
    bool CanReload() const;
    void Reload();
    void AddReserveAmmo(int32 Amount);
};
