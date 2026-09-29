#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheUnitTypes.h"
#include "TUExecutionTypes.h"
#include "TUWeaponComponent.generated.h"

/** Sole mutable ledger for its owning weapon. Actor is the public authority boundary. */
UCLASS(NotBlueprintable)
class THEUNIT_API UTUWeaponComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UTUWeaponComponent();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
 friend class ATU_WeaponBase;
 UPROPERTY(EditDefaultsOnly) FWeaponDefinition WeaponDefinition;
 UPROPERTY(EditDefaultsOnly) FAmmoDefinition AmmoDefinition;
 // Constructor-only seeds; never consulted after hydration/import.
 UPROPERTY(EditDefaultsOnly) FMagazineState MagazineState;
 UPROPERTY(EditDefaultsOnly) int32 AmmoReserve = 90;
 UPROPERTY(ReplicatedUsing=OnRep_PresentationState) FTUItemLedger Ledger;
 UPROPERTY(ReplicatedUsing=OnRep_PresentationState) FTUWeaponActionState Action;
 UFUNCTION() void OnRep_PresentationState();
 bool bHydrated = false;
 TSet<FGuid> UsedActionIds;
 void Hydrate();
 bool ValidateImport(const FTUItemLedger& Value) const;
 bool Import(const FTUItemLedger& Value);
 FTUMagazineInstance* FindMagazine(FGuid Id);
 const FTUMagazineInstance* FindMagazine(FGuid Id) const;
 bool HasAmmo() const;
 bool ConsumeRound();
 bool Cycle();
 bool BeginReload(FGuid Id, int32 Revision, ETUReloadPolicy Policy);
 bool Commit(FGuid Id, int32 Revision);
 void Interrupt();
 FMagazineState Snapshot() const;
 int32 Reserve() const;
};
