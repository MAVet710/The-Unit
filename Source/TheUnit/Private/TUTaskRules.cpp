#include "TUTaskRules.h"
namespace
{
TArray<FTUTaskStep> StepsFor(const FTUTaskDefinition& D)
{
    if(!D.Steps.IsEmpty()) return D.Steps;
    FTUTaskStep S; S.Condition=D.Condition; S.TargetId=D.TargetId; S.RequiredCount=D.RequiredCount; return {S};
}
bool Complete(const TArray<int32>& Counts,const TArray<FTUTaskStep>& Steps)
{
    if(Counts.Num()!=Steps.Num()) return false;
    for(int32 I=0;I<Steps.Num();++I) if(Steps[I].RequiredCount<=0 || Counts[I]<Steps[I].RequiredCount) return false;
    return true;
}
int32 Sum(const TArray<int32>& Counts) { int32 N=0; for(int32 V:Counts) N+=V; return N; }
}
bool FTUTaskRules::Record(FTUTaskProgress& T,const FGuid& Raid,ETUTaskCondition Condition,FName Target,int32 Amount)
{
    if(!Raid.IsValid() || T.bCompleted || Amount<=0 || T.Definition.Policy==ETUTaskPolicy::PhysicalHandover) return false;
    const TArray<FTUTaskStep> Steps=StepsFor(T.Definition);
    bool Matches=false;
    for(const FTUTaskStep& S:Steps) if(S.RequiredCount>0 && S.Condition==Condition && (S.TargetId.IsNone() || S.TargetId==Target)) Matches=true;
    if(!Matches) return false;
    const bool Cumulative=T.Definition.Policy==ETUTaskPolicy::Cumulative;
    if(!Cumulative && T.PendingRaidId!=Raid) { T.PendingSteps.Init(0,Steps.Num()); T.PendingCount=0; T.PendingRaidId=Raid; }
    TArray<int32>& Counts=Cumulative?T.CommittedSteps:T.PendingSteps;
    if(Counts.Num()!=Steps.Num())
    {
        Counts.Init(0,Steps.Num());
        if(Steps.Num()==1) Counts[0]=Cumulative?T.CommittedCount:T.PendingCount;
    }
    for(int32 I=0;I<Steps.Num();++I)
        if(Steps[I].Condition==Condition && (Steps[I].TargetId.IsNone() || Steps[I].TargetId==Target))
            Counts[I]+=FMath::Min(Amount,FMath::Max(0,Steps[I].RequiredCount-Counts[I]));
    if(Cumulative) { T.CommittedCount=Sum(Counts); T.bCompleted=Complete(Counts,Steps); }
    else
    {
        T.PendingCount=Sum(Counts);
        if(T.Definition.Policy==ETUTaskPolicy::SameRaid && Complete(Counts,Steps))
        { T.CommittedSteps=Counts; T.CommittedCount=T.PendingCount; T.bCompleted=true; T.PendingSteps.Init(0,Steps.Num()); T.PendingCount=0; }
    }
    return true;
}
void FTUTaskRules::Resolve(FTUTaskProgress& T,const FTURaidOutcome& O)
{
    if(O.bTraining || T.PlayerId!=O.PlayerId || T.PendingRaidId!=O.RaidId) return;
    const TArray<FTUTaskStep> Steps=StepsFor(T.Definition);
    if(T.Definition.Policy==ETUTaskPolicy::ExtractRequired && O.Outcome==ETURaidPlayerOutcome::Extracted &&
        (T.Definition.RequiredExtractId.IsNone() || T.Definition.RequiredExtractId==O.ExtractId))
    {
        if(T.CommittedSteps.Num()!=Steps.Num()) T.CommittedSteps.Init(0,Steps.Num());
        if(T.PendingSteps.Num()!=Steps.Num()) { T.PendingSteps.Init(0,Steps.Num()); if(Steps.Num()==1) T.PendingSteps[0]=T.PendingCount; }
        for(int32 I=0;I<Steps.Num();++I) T.CommittedSteps[I]=FMath::Min(Steps[I].RequiredCount,T.CommittedSteps[I]+T.PendingSteps[I]);
        T.CommittedCount=Sum(T.CommittedSteps); T.bCompleted=Complete(T.CommittedSteps,Steps);
    }
    T.PendingSteps.Init(0,Steps.Num()); T.PendingCount=0; T.PendingRaidId.Invalidate();
}
bool FTUTaskRules::Handover(FTUTaskProgress& T,FTUItemLedger& Ledger,const FGuid& ItemId)
{
    if(T.bCompleted || T.Definition.Policy!=ETUTaskPolicy::PhysicalHandover || T.Definition.RequiredCount<=0 || T.ConsumedItemIds.Contains(ItemId)) return false;
    FTUItemInstance* Item=Ledger.Items.FindByPredicate([&](const FTUItemInstance& I){return I.InstanceId==ItemId;});
    if(!Item || Item->OwnerId!=T.PlayerId || !Item->bExtracted || !Item->FoundInRaidId.IsValid() || Item->DefinitionId!=T.Definition.TargetId ||
        (!T.Definition.RequiredExtractId.IsNone() && Item->ExtractedAtId!=T.Definition.RequiredExtractId) ||
        (Item->Location!=ETUItemLocation::Carried && Item->Location!=ETUItemLocation::Stash)) return false;
    Item->Location=ETUItemLocation::Consumed; T.ConsumedItemIds.Add(ItemId); ++T.CommittedCount; ++Ledger.Revision;
    T.bCompleted=T.CommittedCount>=T.Definition.RequiredCount; return true;
}
