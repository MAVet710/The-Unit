#pragma once

#include "CoreMinimal.h"
#include "TU_WeaponBase.h"
#include "TU_AK105.generated.h"

/**
 * AK-105 visual/gameplay profile for The Unit.
 * Reuses the shared modular weapon runtime; tuning is gameplay-oriented rather than a manufacturing simulation.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_AK105 : public ATU_WeaponBase
{
    GENERATED_BODY()

public:
    ATU_AK105();
};
