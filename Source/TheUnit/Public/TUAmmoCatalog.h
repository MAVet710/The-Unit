#pragma once
#include "CoreMinimal.h"
#include "TheUnitTypes.h"
#include "TUAmmoCatalog.generated.h"
UCLASS()
class THEUNIT_API UTUAmmoCatalog : public UObject
{
    GENERATED_BODY()
public:
    static TArray<FAmmoDefinition> BetaDefinitions();
    static bool Find(FName AmmoId, FAmmoDefinition& Out);
};
