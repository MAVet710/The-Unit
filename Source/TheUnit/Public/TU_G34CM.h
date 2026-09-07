#pragma once

#include "CoreMinimal.h"
#include "TU_WeaponBase.h"
#include "TU_G34CM.generated.h"

/**
 * Competition/tactical pistol profile inspired by the supplied G34 Combat Master reference.
 * Uses fictionalized gameplay tuning and the shared modular weapon runtime.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_G34CM : public ATU_WeaponBase
{
    GENERATED_BODY()

public:
    ATU_G34CM();
};
