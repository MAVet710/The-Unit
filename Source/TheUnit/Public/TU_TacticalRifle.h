#pragma once

#include "CoreMinimal.h"
#include "TU_WeaponBase.h"
#include "TU_TacticalRifle.generated.h"

/**
 * Default modular first-person rifle for The Unit.
 * Uses fictionalized gameplay tuning while matching the supplied reference configuration visually.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_TacticalRifle : public ATU_WeaponBase
{
    GENERATED_BODY()

public:
    ATU_TacticalRifle();
};
