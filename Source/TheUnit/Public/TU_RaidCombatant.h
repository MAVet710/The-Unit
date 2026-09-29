#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TU_WeaponBase.h"
#include "TUEnemyAIController.h"
#include "TU_RaidCombatant.generated.h"
class UCapsuleComponent;
class USkeletalMeshComponent;
class UTUHealthComponent;

/** Stationary benchmark sentry: bounded sight/hearing and the same finite weapon API as players. */
UCLASS()
class THEUNIT_API ATU_RaidCombatant : public APawn
{
    GENERATED_BODY()
public:
    ATU_RaidCombatant();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer) override;
    ATU_WeaponBase* GetWeapon() const { return Weapon; }
    bool HasLineOfSightTo(const APawn* Pawn) const;
    UPROPERTY(EditAnywhere) float SightRangeCm = 2500.f;
    UPROPERTY(EditAnywhere) float HearingRangeCm = 3500.f;
    UPROPERTY(EditAnywhere) ETUAIArchetype Archetype = ETUAIArchetype::Rifleman;
private:
    UPROPERTY() TObjectPtr<UCapsuleComponent> Capsule;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<UTUHealthComponent> Health;
    UPROPERTY() TObjectPtr<ATU_WeaponBase> Weapon;
    UFUNCTION() void HearShot(FTUWeaponShotResult Shot);
    UFUNCTION() void OnKilled(AActor* DeadActor);
    void Think();
    FTimerHandle ThinkTimer;
    TWeakObjectPtr<APawn> LastAttacker;
    FGuid DeathEventId;
};
