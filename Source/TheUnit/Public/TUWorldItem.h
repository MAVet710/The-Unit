#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TUExecutionTypes.h"
#include "TUWorldItem.generated.h"
class ATU_WeaponBase;
class UStaticMeshComponent;
UCLASS()
class THEUNIT_API ATUWorldItem : public AActor
{
 GENERATED_BODY()
public:
 ATUWorldItem();
 virtual void Tick(float DeltaSeconds) override;
 UStaticMeshComponent* GetMagazineVisual() const { return MagazineVisual; }
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 void InitializeMagazine(const FTUMagazineInstance& Value);
 bool TryPickup(ATU_WeaponBase* Destination);
 FTUMagazineInstance GetMagazine() const { return Magazine; }
private:
 UPROPERTY(ReplicatedUsing=RefreshVisual) FTUMagazineInstance Magazine;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> MagazineVisual;
 UFUNCTION() void RefreshVisual();
 FVector DropVelocity = FVector::ZeroVector;
 bool bSettled = false;
 UPROPERTY(Replicated) bool bClaimed = false;
};
