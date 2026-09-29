#include "TUWorldItem.h"
#include "TUInventoryAmmoRegistry.h"
#include "TU_WeaponBase.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
ATUWorldItem::ATUWorldItem()
{
 bReplicates = true; SetReplicateMovement(true); PrimaryActorTick.bCanEverTick = true;
 auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("PickupBounds")); Shape->InitSphereRadius(12.f); Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic")); Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); SetRootComponent(Shape);
 MagazineVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineVisual"));
 MagazineVisual->SetupAttachment(Shape); MagazineVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> M(TEXT("/Game/TheUnit/Weapons/PrototypeRifle/SM_TURifleMagazine.SM_TURifleMagazine"));
 if (M.Succeeded()) { MagazineVisual->SetStaticMesh(M.Object); MagazineVisual->SetRelativeLocation(-M.Object->GetBounds().Origin); }
 MagazineVisual->SetVisibility(false);
}
void ATUWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ATUWorldItem, Magazine); DOREPLIFETIME(ATUWorldItem, bClaimed);
}
void ATUWorldItem::InitializeMagazine(const FTUMagazineInstance& Value) { if (HasAuthority() && !Magazine.InstanceId.IsValid()) { Magazine = Value; Magazine.OwnerId.Invalidate(); Magazine.Location = ETUItemLocation::Ground; RefreshVisual(); ForceNetUpdate(); } }
bool ATUWorldItem::TryPickup(ATU_WeaponBase* Destination)
{
 if (!HasAuthority() || bClaimed || !IsValid(Destination) || !Destination->HasAuthority() || FVector::DistSquared(Destination->GetActorLocation(),GetActorLocation()) > FMath::Square(250.f)) return false;
 // Game-thread compare-and-transfer; mark claim before notifying the destination.
 if (!TUInventoryAmmo::ValidMagazine(Magazine)) return false;
 if (const auto* Operator = Cast<ATU_ArmedOperatorCharacter>(Destination->GetOwner()); Operator && (Operator->IsCombatDisabled() || Operator->GetCurrentWeapon() != Destination)) return false;
 const auto DestinationLedger = Destination->ExportItemLedger();
 if (DestinationLedger.Weapons.Num() != 1 || Destination->GetActionState().bActive || DestinationLedger.Magazines.ContainsByPredicate([this](const FTUMagazineInstance& M){ return M.InstanceId == Magazine.InstanceId; })) return false;
 if (UGameInstance* GI = GetWorld()->GetGameInstance()) {
  if (auto* Lifecycle = GI->GetSubsystem<UTUHideoutLifecycleSubsystem>()) {
   const FGuid PlayerId = DestinationLedger.Weapons[0].OwnerId;
   const FGuid RaidId = Lifecycle->GetActiveRaidIdForPlayer(PlayerId);
   if (RaidId.IsValid()) {
    FTUItemLedger Acquired; auto Granted = Magazine; Granted.OwnerId = PlayerId; Granted.WeaponId = DestinationLedger.Weapons[0].InstanceId; Granted.Location = ETUItemLocation::Carried; Acquired.Magazines.Add(Granted);
    if (!Lifecycle->RecordRaidAcquisition(RaidId,PlayerId,Acquired)) return false;
   }
  }
 }
 bClaimed = true;
 if (!Destination->AcceptGroundMagazine(Magazine)) { bClaimed = false; return false; }
 Destroy(); return true;
}


void ATUWorldItem::RefreshVisual()
{
 MagazineVisual->SetVisibility(!bClaimed && Magazine.InstanceId.IsValid() && Magazine.Capacity == 30 && TUInventoryAmmo::Compatible(Magazine.CompatibleAmmoId, TEXT("Ammo_TU556_Ball")));
}
void ATUWorldItem::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (!HasAuthority() || !Magazine.InstanceId.IsValid() || bClaimed || bSettled || !GetWorld()) return;
 const float Dt = FMath::Clamp(DeltaSeconds, 0.f, .1f);
 DropVelocity.Z -= 980.f * Dt;
 FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(TUDroppedMagazine), false, this);
 const FVector Next = GetActorLocation() + DropVelocity * Dt;
 if (GetWorld()->SweepSingleByObjectType(Hit,GetActorLocation(),Next,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeSphere(6.f),Params)) {
  SetActorLocation(Hit.Location); DropVelocity = FVector::ZeroVector; bSettled = true; ForceNetUpdate();
 } else SetActorLocation(Next);
}
