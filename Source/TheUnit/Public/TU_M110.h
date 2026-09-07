#pragma once

#include "CoreMinimal.h"
#include "TU_WeaponBase.h"
#include "TU_M110.generated.h"

/**
 * Semi-auto precision/DMR profile inspired by the supplied M110 reference.
 * Uses fictionalized gameplay tuning and the shared modular weapon runtime.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_M110 : public ATU_WeaponBase
{
    GENERATED_BODY()

public:
    ATU_M110();
};
