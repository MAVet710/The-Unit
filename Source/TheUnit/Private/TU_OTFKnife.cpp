#include "TU_OTFKnife.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ATU_OTFKnife::ATU_OTFKnife()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
    HandleMesh->SetupAttachment(Root);
    HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BladeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BladeMesh"));
    BladeMesh->SetupAttachment(Root);
    BladeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATU_OTFKnife::BeginPlay()
{
    Super::BeginPlay();
    BladeState = ETUOTFBladeState::Retracted;
    SetBladeAlpha(0.0f);
}

void ATU_OTFKnife::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (BladeState != ETUOTFBladeState::Deploying && BladeState != ETUOTFBladeState::Retracting)
    {
        return;
    }

    const float Duration = BladeState == ETUOTFBladeState::Deploying
        ? DeploymentDurationSeconds
        : RetractionDurationSeconds;

    MotionElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
    const float TimeAlpha = FMath::Clamp(MotionElapsedSeconds / FMath::Max(Duration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
    const float EasedAlpha = FMath::InterpEaseOut(MotionStartAlpha, MotionTargetAlpha, TimeAlpha, 3.0f);
    SetBladeAlpha(EasedAlpha);

    if (TimeAlpha >= 1.0f)
    {
        SetBladeAlpha(MotionTargetAlpha);
        CompleteBladeMotion();
    }
}

bool ATU_OTFKnife::EquipTo(USkeletalMeshComponent* ParentMesh, FName SocketName)
{
    if (!ParentMesh)
    {
        return false;
    }

    AttachToComponent(ParentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(false);

    BladeState = ETUOTFBladeState::Retracted;
    SetBladeAlpha(0.0f);
    DeployBlade();
    return true;
}

void ATU_OTFKnife::DeployBlade()
{
    if (BladeState == ETUOTFBladeState::Deployed || BladeState == ETUOTFBladeState::Deploying)
    {
        return;
    }

    BeginBladeMotion(ETUOTFBladeState::Deploying, 1.0f);
}

void ATU_OTFKnife::RetractBlade()
{
    if (BladeState == ETUOTFBladeState::Retracted || BladeState == ETUOTFBladeState::Retracting)
    {
        return;
    }

    BeginBladeMotion(ETUOTFBladeState::Retracting, 0.0f);
}

void ATU_OTFKnife::ToggleBlade()
{
    if (BladeState == ETUOTFBladeState::Deployed || BladeState == ETUOTFBladeState::Deploying)
    {
        RetractBlade();
    }
    else
    {
        DeployBlade();
    }
}

bool ATU_OTFKnife::PerformMeleeAttack()
{
    if (BladeState != ETUOTFBladeState::Deployed || !GetWorld())
    {
        return false;
    }

    FVector TraceStart = GetActorLocation();
    FRotator ViewRotation = GetActorRotation();
    APawn* OwnerPawn = Cast<APawn>(GetOwner());

    if (OwnerPawn)
    {
        if (AController* Controller = OwnerPawn->GetController())
        {
            Controller->GetPlayerViewPoint(TraceStart, ViewRotation);
        }
        else
        {
            OwnerPawn->GetActorEyesViewPoint(TraceStart, ViewRotation);
        }
    }

    const FVector Direction = ViewRotation.Vector();
    const FVector TraceEnd = TraceStart + Direction * MeleeRangeCm;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TUOTFMeleeSweep), false, this);
    QueryParams.AddIgnoredActor(this);
    if (GetOwner())
    {
        QueryParams.AddIgnoredActor(GetOwner());
    }

    FHitResult Hit;
    const bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeSphere(MeleeSweepRadiusCm),
        QueryParams);

    if (!bHit || !Hit.GetActor())
    {
        return false;
    }

    AController* InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
    UGameplayStatics::ApplyPointDamage(
        Hit.GetActor(),
        MeleeDamage,
        Direction,
        Hit,
        InstigatorController,
        this,
        UDamageType::StaticClass());

    OnMeleeHit.Broadcast(Hit.GetActor());
    return true;
}

void ATU_OTFKnife::BeginBladeMotion(ETUOTFBladeState NewState, float TargetAlpha)
{
    MotionStartAlpha = BladeAlpha;
    MotionTargetAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);
    MotionElapsedSeconds = 0.0f;
    BladeState = NewState;
}

void ATU_OTFKnife::SetBladeAlpha(float NewAlpha)
{
    BladeAlpha = FMath::Clamp(NewAlpha, 0.0f, 1.0f);
    if (BladeMesh)
    {
        BladeMesh->SetRelativeLocation(FMath::Lerp(RetractedBladeLocation, DeployedBladeLocation, BladeAlpha));
    }
}

void ATU_OTFKnife::CompleteBladeMotion()
{
    if (MotionTargetAlpha >= 1.0f - KINDA_SMALL_NUMBER)
    {
        BladeState = ETUOTFBladeState::Deployed;
        OnBladeDeployed.Broadcast();
    }
    else
    {
        BladeState = ETUOTFBladeState::Retracted;
        OnBladeRetracted.Broadcast();
    }
}
