#pragma once

#include "CoreMinimal.h"
#include "TU_WeaponBase.h"
#include "TU_RGRFive7.generated.h"

/**
 * Semi-auto 5.7-style service/tactical pistol profile inspired by the supplied RGR Five7 reference.
 * Uses fictionalized gameplay tuning and the shared modular weapon runtime.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_RGRFive7 : public ATU_WeaponBase
{
    GENERATED_BODY()

public:
    ATU_RGRFive7();
};
