#pragma once

#include "CoreMinimal.h"
#include "TUWeaponInstanceState.h"

class UTUWeaponDefinitionCatalog;

/** Validation rules for persistent weapon-instance snapshots. Owns no item or runtime weapon state. */
struct THEUNIT_API FTUWeaponInstanceRules
{
    static bool ValidateInstance(
        const FWeaponInstanceState& Instance,
        const UTUWeaponDefinitionCatalog* Catalog,
        FString& OutFailureReason);
};
