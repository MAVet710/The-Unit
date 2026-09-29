#pragma once
#include "CoreMinimal.h"
#include "TUExecutionTypes.h"

namespace TUPersistence
{
    THEUNIT_API bool ValidateLedger(const FTUItemLedger& Ledger, const FGuid& OwnerId);
    THEUNIT_API bool ContainsExact(const FTUItemLedger& Stash, const FTUItemLedger& Selected);
    THEUNIT_API void Remove(FTUItemLedger& Stash, const FTUItemLedger& Selected);
    THEUNIT_API void Append(FTUItemLedger& Target, const FTUItemLedger& Source);
    THEUNIT_API FTUItemLedger OwnedBy(const FTUItemLedger& Ledger, const FGuid& PlayerId);
    THEUNIT_API bool ValidateReturn(const FTUItemLedger& Returned, const FTUItemLedger& Deployed, const FGuid& PlayerId, const FGuid& RaidId);
}
