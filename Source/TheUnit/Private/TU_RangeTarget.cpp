#include "TU_RangeTarget.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATU_RangeTarget::ATU_RangeTarget()
{
    PrimaryActorTick.bCanEverTick = false;

    TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
    SetRootComponent(TargetMesh);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        TargetMesh->SetStaticMesh(CubeFinder.Object);
        TargetMesh->SetRelativeScale3D(FVector(0.15f, 0.45f, 0.9f));
    }

    TargetMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

void ATU_RangeTarget::BeginPlay()
{
    Super::BeginPlay();
    ResetTarget();
    ResetCount = 0;
}

float ATU_RangeTarget::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (DamageAmount <= 0.0f || CurrentHealth <= 0.0f)
    {
        return 0.0f;
    }

    const float Applied = FMath::Min(DamageAmount, CurrentHealth);
    CurrentHealth -= Applied;
    ++HitCount;

    if (CurrentHealth <= KINDA_SMALL_NUMBER)
    {
        CurrentHealth = 0.0f;
        TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TargetMesh->SetVisibility(false, true);

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ResetTimerHandle,
                this,
                &ATU_RangeTarget::ResetTarget,
                ResetDelaySeconds,
                false);
        }
    }

    return Applied;
}

void ATU_RangeTarget::ResetTarget()
{
    CurrentHealth = MaxHealth;
    TargetMesh->SetVisibility(true, true);
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ++ResetCount;
}
