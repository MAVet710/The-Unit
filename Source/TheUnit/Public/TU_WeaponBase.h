#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_WeaponBase.generated.h"

/**
 * Base weapon actor.
 * Will define shared weapon interfaces for firing, reloading, and replication-safe weapon state.
 */
UCLASS()
class THEUNIT_API ATU_WeaponBase : public AActor
{
    GENERATED_BODY()
};
