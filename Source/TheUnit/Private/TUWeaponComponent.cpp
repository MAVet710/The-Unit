#include "TUWeaponComponent.h"
#include "TUInventoryAmmoRegistry.h"
#include "TU_WeaponBase.h"
#include "TUWeaponPartsComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UTUWeaponComponent::UTUWeaponComponent()
{
 PrimaryComponentTick.bCanEverTick = false;
 SetIsReplicatedByDefault(true);
 MagazineState.RoundsInMagazine = 29;
 WeaponDefinition.bSemiAutoOnly = false;
 WeaponDefinition.FireRateRPM = 600.f;
 AmmoDefinition.Damage = 25.f;
}
void UTUWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(UTUWeaponComponent, Ledger);
 DOREPLIFETIME(UTUWeaponComponent, Action);
}
void UTUWeaponComponent::OnRep_PresentationState()
{
 // Wait for a coherent replicated ledger/action pair before replacing visuals.
 if (Ledger.Revision != Action.Revision) return;
 if (auto* Weapon = Cast<ATU_WeaponBase>(GetOwner()); Weapon && Weapon->PartsPresentation)
  Weapon->PartsPresentation->UpdatePresentation(0.f);
}
void UTUWeaponComponent::Hydrate()
{
 if (bHydrated || Ledger.Weapons.Num()) { bHydrated = true; return; }
 if (GetOwner() && !GetOwner()->HasAuthority()) return;
 bHydrated = true;
 FWeaponInstanceState W;
 W.InstanceId = FGuid::NewGuid(); W.OwnerId = FGuid::NewGuid(); W.DefinitionId = WeaponDefinition.WeaponId;
 W.ChamberAmmoId = MagazineState.bRoundChambered ? AmmoDefinition.AmmoId : NAME_None;
 Ledger.Weapons.Add(W); Action.WeaponId = W.InstanceId;
 int32 Remaining = FMath::Clamp(AmmoReserve, 0, 10000);
 bool bFirst = true;
 do {
  FTUMagazineInstance M; M.InstanceId = FGuid::NewGuid(); M.OwnerId = W.OwnerId;
  M.WeaponId = W.InstanceId; M.Capacity = FMath::Clamp(MagazineState.Capacity, 1, 500); M.CompatibleAmmoId = WeaponDefinition.CompatibleAmmoId;
  const int32 Count = bFirst ? FMath::Clamp(MagazineState.RoundsInMagazine, 0, M.Capacity) : FMath::Min(Remaining, M.Capacity);
  M.Cartridges.Init(AmmoDefinition.AmmoId, Count);
  if (bFirst) { M.Location = ETUItemLocation::Inserted; M.WeaponId = W.InstanceId; Ledger.Weapons[0].InsertedMagazineId = M.InstanceId; }
  else Remaining -= Count;
  Ledger.Magazines.Add(M); bFirst = false;
 } while (Remaining > 0);
}
FTUMagazineInstance* UTUWeaponComponent::FindMagazine(FGuid Id) { return Ledger.Magazines.FindByPredicate([Id](const FTUMagazineInstance& M){ return M.InstanceId == Id; }); }
const FTUMagazineInstance* UTUWeaponComponent::FindMagazine(FGuid Id) const { return Ledger.Magazines.FindByPredicate([Id](const FTUMagazineInstance& M){ return M.InstanceId == Id; }); }
bool UTUWeaponComponent::ValidateImport(const FTUItemLedger& Value) const
{
 if (Value.Weapons.Num() != 1 || !Value.Weapons[0].InstanceId.IsValid() || !Value.Weapons[0].OwnerId.IsValid()) return false;
 const auto& W = Value.Weapons[0];
 if (!FMath::IsFinite(W.ConditionNormalized) || W.ConditionNormalized < 0.f || W.ConditionNormalized > 1.f || W.DefinitionId != WeaponDefinition.WeaponId || Value.Revision < 0) return false;
 if (!W.ChamberAmmoId.IsNone() && !TUInventoryAmmo::Compatible(W.ChamberAmmoId,WeaponDefinition.CompatibleAmmoId)) return false;
 for (FName Round : Value.LooseCartridges) if (TUInventoryAmmo::Family(Round).IsNone()) return false;
 TSet<FGuid> Ids; Ids.Add(W.InstanceId); bool bInserted = !W.InsertedMagazineId.IsValid();
 for (const auto& M : Value.Magazines) {
  if (M.CompatibleAmmoId.IsNone() || !M.InstanceId.IsValid() || Ids.Contains(M.InstanceId) || M.OwnerId != W.OwnerId || M.Capacity <= 0 || M.Capacity > 500 || M.Cartridges.Num() > M.Capacity || M.Location == ETUItemLocation::Ground) return false;
  if (!TUInventoryAmmo::ValidMagazine(M)) return false;
  Ids.Add(M.InstanceId);
  if (M.InstanceId == W.InsertedMagazineId) { if (M.Location != ETUItemLocation::Inserted || M.WeaponId != W.InstanceId || !TUInventoryAmmo::Compatible(M.CompatibleAmmoId, WeaponDefinition.CompatibleAmmoId)) return false; bInserted = true; }
  else if (M.Location == ETUItemLocation::Inserted || (M.WeaponId.IsValid() && M.WeaponId != W.InstanceId)) return false;
 }
 for (const auto& I : Value.Items) { if (!I.InstanceId.IsValid() || Ids.Contains(I.InstanceId) || I.OwnerId != W.OwnerId) return false; Ids.Add(I.InstanceId); }
 if (!bInserted || Value.WeaponActions.Num() > 1) return false;
 if (Value.WeaponActions.Num()) {
  const auto& A = Value.WeaponActions[0];
  if (A.WeaponId != W.InstanceId || static_cast<uint8>(A.Phase) > static_cast<uint8>(ETUWeaponActionPhase::Chambered) || static_cast<uint8>(A.Policy) > static_cast<uint8>(ETUReloadPolicy::Drop)) return false;
  if (A.Phase != ETUWeaponActionPhase::Ready) {
   const auto* Replacement = Value.Magazines.FindByPredicate([&A](const FTUMagazineInstance& M){ return M.InstanceId == A.ReplacementMagazineId; });
   if (!Replacement || !A.ActionId.IsValid() || !TUInventoryAmmo::Compatible(Replacement->CompatibleAmmoId,WeaponDefinition.CompatibleAmmoId)) return false;
   if ((A.Phase == ETUWeaponActionPhase::Inserted || A.Phase == ETUWeaponActionPhase::Chambered) && W.InsertedMagazineId != Replacement->InstanceId) return false;
   if (A.Phase == ETUWeaponActionPhase::Acquired && Replacement->Location != ETUItemLocation::InHand) return false;
   if ((A.Phase == ETUWeaponActionPhase::Removed || A.Phase == ETUWeaponActionPhase::Acquired) && W.InsertedMagazineId.IsValid()) return false;
   if (A.Phase == ETUWeaponActionPhase::Begin && (W.InsertedMagazineId != A.OutgoingMagazineId || Replacement->InstanceId == W.InsertedMagazineId)) return false;
  }
 }
 return true;
}
bool UTUWeaponComponent::Import(const FTUItemLedger& Value)
{
 if (!ValidateImport(Value)) return false;
 const auto& W = Value.Weapons[0];
 Ledger = Value; bHydrated = true;
  Action = FTUWeaponActionState();
 if (Value.WeaponActions.Num() == 1 && Value.WeaponActions[0].WeaponId == W.InstanceId) Action = Value.WeaponActions[0];
 Action.WeaponId = W.InstanceId; Action.bActive = false; Action.Revision = Ledger.Revision;
 for (auto& M : Ledger.Magazines) M.WeaponId = W.InstanceId;
 if (Action.ActionId.IsValid()) UsedActionIds.Add(Action.ActionId);
 return true;
}
bool UTUWeaponComponent::HasAmmo() const { return Ledger.Weapons.Num() == 1 && !Action.bActive && Action.Phase == ETUWeaponActionPhase::Ready && !Ledger.Weapons[0].bActionOpen && !Ledger.Weapons[0].ChamberAmmoId.IsNone(); }
bool UTUWeaponComponent::ConsumeRound()
{
 if (!HasAmmo()) return false;
 auto& W = Ledger.Weapons[0]; W.ChamberAmmoId = NAME_None;
 if (auto* M = FindMagazine(W.InsertedMagazineId); M && M->Cartridges.Num()) { W.ChamberAmmoId = M->Cartridges[0]; M->Cartridges.RemoveAt(0); }
 else W.bActionOpen = true;
 ++Ledger.Revision; Action.Revision = Ledger.Revision; return true;
}
bool UTUWeaponComponent::Cycle()
{
 if (Action.bActive || Action.Phase != ETUWeaponActionPhase::Ready || Ledger.Weapons.Num() != 1) return false;
 auto& W = Ledger.Weapons[0];
 // Explicit catch-and-retain cycle policy: the live cartridge remains carried, never vanishes.
 if (!W.ChamberAmmoId.IsNone()) Ledger.LooseCartridges.Add(W.ChamberAmmoId);
 W.ChamberAmmoId = NAME_None;
 if (auto* M = FindMagazine(W.InsertedMagazineId); M && M->Cartridges.Num()) { W.ChamberAmmoId = M->Cartridges[0]; M->Cartridges.RemoveAt(0); }
 W.bActionOpen = W.ChamberAmmoId.IsNone(); ++Ledger.Revision; Action.Revision = Ledger.Revision; return true;
}
bool UTUWeaponComponent::BeginReload(FGuid Id, int32 Revision, ETUReloadPolicy Policy)
{
 if (static_cast<uint8>(Policy) > static_cast<uint8>(ETUReloadPolicy::Drop) || !Id.IsValid() || UsedActionIds.Contains(Id) || Revision != Ledger.Revision || Action.bActive || Ledger.Weapons.Num() != 1) return false;
 // Resume the exact committed phase after interruption; never roll back a transfer.
 if (Action.Phase == ETUWeaponActionPhase::Ready) {
  const auto* Replacement = Ledger.Magazines.FindByPredicate([this](const FTUMagazineInstance& M){ return (M.Location == ETUItemLocation::Carried || M.Location == ETUItemLocation::InHand) && M.Cartridges.Num() > 0 && TUInventoryAmmo::Compatible(M.CompatibleAmmoId,WeaponDefinition.CompatibleAmmoId); });
  if (!Replacement) return false;
  Action.ReplacementMagazineId = Replacement->InstanceId;
  Action.OutgoingMagazineId = Ledger.Weapons[0].InsertedMagazineId;
  Action.Phase = ETUWeaponActionPhase::Begin; Action.Policy = Policy;
  Action.bRequiresChamberCycle = Ledger.Weapons[0].ChamberAmmoId.IsNone();
 }
 Action.bRequiresChamberCycle |= Ledger.Weapons[0].ChamberAmmoId.IsNone();
 UsedActionIds.Add(Id); Action.ActionId = Id; Action.bActive = true;
 ++Ledger.Revision; Action.Revision = Ledger.Revision; return true;
}
bool UTUWeaponComponent::Commit(FGuid Id, int32 Revision)
{
 if (!Action.bActive || Id != Action.ActionId || Revision != Ledger.Revision) return false;
 auto& W = Ledger.Weapons[0];
 switch(Action.Phase) {
 case ETUWeaponActionPhase::Begin:
  if (auto* M = FindMagazine(W.InsertedMagazineId)) { M->Location = Action.Policy == ETUReloadPolicy::Drop ? ETUItemLocation::Ground : ETUItemLocation::Carried; }
  W.InsertedMagazineId.Invalidate(); Action.Phase = ETUWeaponActionPhase::Removed; break;
 case ETUWeaponActionPhase::Removed:
  if (auto* M = FindMagazine(Action.ReplacementMagazineId)) M->Location = ETUItemLocation::InHand; else return false;
  Action.Phase = ETUWeaponActionPhase::Acquired; break;
 case ETUWeaponActionPhase::Acquired:
  if (auto* M = FindMagazine(Action.ReplacementMagazineId)) { M->Location = ETUItemLocation::Inserted; M->WeaponId = W.InstanceId; W.InsertedMagazineId = M->InstanceId; } else return false;
  Action.Phase = ETUWeaponActionPhase::Inserted; break;
 case ETUWeaponActionPhase::Inserted:
  if (W.ChamberAmmoId.IsNone()) { if (auto* M = FindMagazine(W.InsertedMagazineId); M && M->Cartridges.Num()) { W.ChamberAmmoId = M->Cartridges[0]; M->Cartridges.RemoveAt(0); } }
  W.bActionOpen = W.ChamberAmmoId.IsNone(); Action.Phase = ETUWeaponActionPhase::Chambered; break;
 case ETUWeaponActionPhase::Chambered: Action.Phase = ETUWeaponActionPhase::Ready; Action.bActive = false; break;
 default: return false;
 }
 ++Ledger.Revision; Action.Revision = Ledger.Revision; return true;
}
void UTUWeaponComponent::Interrupt() { if (Action.bActive) { Action.bActive = false; ++Ledger.Revision; Action.Revision = Ledger.Revision; } }
FMagazineState UTUWeaponComponent::Snapshot() const
{
 FMagazineState S; S.RoundsInMagazine = 0; S.bRoundChambered = false;
 if (Ledger.Weapons.Num()) { S.bRoundChambered = !Ledger.Weapons[0].ChamberAmmoId.IsNone(); if (const auto* M = FindMagazine(Ledger.Weapons[0].InsertedMagazineId)) { S.Capacity = M->Capacity; S.RoundsInMagazine = M->Cartridges.Num(); } }
 return S;
}
int32 UTUWeaponComponent::Reserve() const { int32 N = Ledger.LooseCartridges.Num(); for (const auto& M : Ledger.Magazines) if (M.Location == ETUItemLocation::Carried || M.Location == ETUItemLocation::InHand) N += M.Cartridges.Num(); return N; }
