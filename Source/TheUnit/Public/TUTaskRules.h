#pragma once
#include "CoreMinimal.h"
#include "TUExecutionTypes.h"
/** Live authority and automation share these transitions. */
struct THEUNIT_API FTUTaskRules
{
    static bool Record(FTUTaskProgress& Task, const FGuid& RaidId, ETUTaskCondition Condition, FName TargetId, int32 Amount);
    static void Resolve(FTUTaskProgress& Task, const FTURaidOutcome& Outcome);
    static bool Handover(FTUTaskProgress& Task, FTUItemLedger& Ledger, const FGuid& ItemId);
};
