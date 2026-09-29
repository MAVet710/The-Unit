#include "TUPersistence.h"

namespace TUPersistence
{
bool ValidateLedger(const FTUItemLedger& L, const FGuid& Owner)
{
    if (!Owner.IsValid() || L.Revision < 0) return false;
    TSet<FGuid> IDs;
    auto Accept = [&IDs, &Owner](const FGuid& ID, const FGuid& ItemOwner)
    {
        if (!ID.IsValid() || ItemOwner != Owner || IDs.Contains(ID)) return false;
        IDs.Add(ID); return true;
    };
    for (const auto& W : L.Weapons)
    {
        if (!Accept(W.InstanceId, W.OwnerId) || W.DefinitionId.IsNone() || !FMath::IsFinite(W.ConditionNormalized) || W.ConditionNormalized < 0 || W.ConditionNormalized > 1) return false;
        if (W.InsertedMagazineId.IsValid())
        {
            const auto* M = L.Magazines.FindByPredicate([&](const auto& V) { return V.InstanceId == W.InsertedMagazineId; });
            if (!M || M->WeaponId != W.InstanceId || M->Location != ETUItemLocation::Inserted) return false;
        }
    }
    for (const auto& M : L.Magazines)
    {
        if (!Accept(M.InstanceId, M.OwnerId) || M.Capacity < 0 || M.Capacity > 1000 || M.Cartridges.Num() > M.Capacity || M.DefinitionId.IsNone()) return false;
        for (const FName Ammo : M.Cartridges) if (Ammo.IsNone()) return false;
        if (M.Location == ETUItemLocation::Inserted)
        {
            const auto* W = L.Weapons.FindByPredicate([&](const auto& V) { return V.InstanceId == M.WeaponId; });
            if (!W || W->InsertedMagazineId != M.InstanceId) return false;
        }
    }
    for (const auto& I : L.Items) if (!Accept(I.InstanceId, I.OwnerId) || I.DefinitionId.IsNone()) return false;
    for (FName Ammo : L.LooseCartridges) if (Ammo.IsNone()) return false;
    return true;
}

template <typename T> bool ExactSubset(const TArray<T>& All, const TArray<T>& Selected)
{
    for (const auto& V : Selected)
    {
        const auto* Existing = All.FindByPredicate([&](const auto& A) { return A.InstanceId == V.InstanceId; });
        if (!Existing || !T::StaticStruct()->CompareScriptStruct(Existing, &V, 0)) return false;
    }
    return true;
}
bool ContainsExact(const FTUItemLedger& Stash, const FTUItemLedger& Selected)
{
    if (!ExactSubset(Stash.Weapons, Selected.Weapons) || !ExactSubset(Stash.Magazines, Selected.Magazines) || !ExactSubset(Stash.Items, Selected.Items)) return false;
    TArray<FName> Loose = Stash.LooseCartridges;
    for (const auto Ammo : Selected.LooseCartridges) { const int32 Index = Loose.Find(Ammo); if (Index == INDEX_NONE) return false; Loose.RemoveAt(Index); }
    return true;
}
void Remove(FTUItemLedger& Stash, const FTUItemLedger& Selected)
{
    for (const auto& W : Selected.Weapons) Stash.WeaponActions.RemoveAll([&](const auto& A) { return A.WeaponId == W.InstanceId; });
    for (const auto& W : Selected.Weapons) Stash.Weapons.RemoveAll([&](const auto& V) { return V.InstanceId == W.InstanceId; });
    for (const auto& M : Selected.Magazines) Stash.Magazines.RemoveAll([&](const auto& V) { return V.InstanceId == M.InstanceId; });
    for (const auto& I : Selected.Items) Stash.Items.RemoveAll([&](const auto& V) { return V.InstanceId == I.InstanceId; });
    for (const auto Ammo : Selected.LooseCartridges) { const int32 Index = Stash.LooseCartridges.Find(Ammo); if (Index != INDEX_NONE) Stash.LooseCartridges.RemoveAt(Index); }
    ++Stash.Revision;
}
void Append(FTUItemLedger& Target, const FTUItemLedger& Source)
{
    Target.Weapons.Append(Source.Weapons); Target.Magazines.Append(Source.Magazines); Target.Items.Append(Source.Items); Target.LooseCartridges.Append(Source.LooseCartridges); Target.WeaponActions.Append(Source.WeaponActions); ++Target.Revision;
}
FTUItemLedger OwnedBy(const FTUItemLedger& Ledger, const FGuid& PlayerId)
{
    FTUItemLedger Result; Result.Revision = Ledger.Revision;
    for (const auto& V : Ledger.Weapons) if (V.OwnerId == PlayerId) Result.Weapons.Add(V);
    for (const auto& V : Ledger.Magazines) if (V.OwnerId == PlayerId) Result.Magazines.Add(V);
    for (const auto& V : Ledger.Items) if (V.OwnerId == PlayerId) Result.Items.Add(V);
    for (const auto& A : Ledger.WeaponActions) if (Result.Weapons.ContainsByPredicate([&](const auto& W) { return W.InstanceId == A.WeaponId; })) Result.WeaponActions.Add(A);
    // Loose cartridges have no OwnerId in schema: prohibit pooled loose balances.
    return Result;
}
bool ValidateReturn(const FTUItemLedger& Returned, const FTUItemLedger& Deployed, const FGuid& Owner, const FGuid& Raid)
{
    if (!ValidateLedger(Returned, Owner)) return false;
    TMap<FName, int32> AmmoBudget;
    auto Count = [&AmmoBudget](const FTUItemLedger& L, int32 Sign)
    {
        for (const auto& W : L.Weapons) if (!W.ChamberAmmoId.IsNone()) AmmoBudget.FindOrAdd(W.ChamberAmmoId) += Sign;
        for (const auto& M : L.Magazines) for (auto Ammo : M.Cartridges) AmmoBudget.FindOrAdd(Ammo) += Sign;
        for (auto Ammo : L.LooseCartridges) AmmoBudget.FindOrAdd(Ammo) += Sign;
    };
    Count(Deployed, 1); Count(Returned, -1);
    for (const auto& Pair : AmmoBudget) if (Pair.Value < 0) return false;
    for (const auto& W : Returned.Weapons)
    {
        const auto* Original = Deployed.Weapons.FindByPredicate([&](const auto& V) { return V.InstanceId == W.InstanceId; });
        if (!Original || Original->DefinitionId != W.DefinitionId || W.ConditionNormalized > Original->ConditionNormalized) return false;
    }
    for (const auto& M : Returned.Magazines)
    {
        const auto* Original = Deployed.Magazines.FindByPredicate([&](const auto& V) { return V.InstanceId == M.InstanceId; });
        if (!Original || Original->DefinitionId != M.DefinitionId || Original->Capacity != M.Capacity || Original->CompatibleAmmoId != M.CompatibleAmmoId) return false;
    }
    for (const auto& I : Returned.Items)
    {
        const auto* Original = Deployed.Items.FindByPredicate([&](const auto& V) { return V.InstanceId == I.InstanceId; });
        if (!Original || Original->DefinitionId != I.DefinitionId || Original->FoundInRaidId != I.FoundInRaidId) return false;
    }
    return true;
}
}
