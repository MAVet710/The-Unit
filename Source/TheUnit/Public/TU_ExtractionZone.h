#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_ExtractionZone.generated.h"

/**
 * Extraction area actor.
 * Will validate extraction conditions and trigger end-of-mission extraction flow.
 */
UCLASS()
class THEUNIT_API ATU_ExtractionZone : public AActor
{
    GENERATED_BODY()
};
