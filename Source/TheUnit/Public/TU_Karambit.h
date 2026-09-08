#pragma once

#include "CoreMinimal.h"
#include "TU_OTFKnife.h"
#include "TU_Karambit.generated.h"

/**
 * Fixed-blade karambit-style melee profile for The Unit.
 * Reuses the shared first-person melee hit path from ATU_OTFKnife, but the
 * blade's authored retracted/deployed transforms are identical so no OTF-style
 * blade travel is presented when drawing or holstering this weapon.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_Karambit : public ATU_OTFKnife
{
    GENERATED_BODY()

public:
    ATU_Karambit();
};
