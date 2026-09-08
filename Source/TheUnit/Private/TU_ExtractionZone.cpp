#include "TU_ExtractionZone.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

ATU_ExtractionZone::ATU_ExtractionZone()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionTrigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(220.0f, 220.0f, 150.0f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATU_ExtractionZone::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ATU_ExtractionZone::HandleBeginOverlap);
    Trigger->OnComponentEndOverlap.AddDynamic(this, &ATU_ExtractionZone::HandleEndOverlap);
}

void ATU_ExtractionZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn || bExtractionPending || !GetWorld())
    {
        return;
    }

    if (bRequireActiveMission)
    {
        UGameInstance* GameInstance = GetGameInstance();
        UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance
            ? GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>()
            : nullptr;
        if (!Lifecycle || !Lifecycle->IsMissionInProgress())
        {
            return;
        }
    }

    PendingPawn = Pawn;
    bExtractionPending = true;

    if (ExtractionHoldSeconds <= KINDA_SMALL_NUMBER)
    {
        FinishTimedExtraction();
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        ExtractionTimer,
        this,
        &ATU_ExtractionZone::FinishTimedExtraction,
        ExtractionHoldSeconds,
        false);
}

void ATU_ExtractionZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!bExtractionPending || OtherActor != PendingPawn.Get() || !GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ExtractionTimer);
    PendingPawn.Reset();
    bExtractionPending = false;
}

void ATU_ExtractionZone::FinishTimedExtraction()
{
    if (!bExtractionPending)
    {
        return;
    }

    ExtractNow(bCountsAsOperationComplete);
}

bool ATU_ExtractionZone::ExtractNow(bool bOperationCompleted)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return false;
    }

    UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>();
    if (!Lifecycle || (bRequireActiveMission && !Lifecycle->IsMissionInProgress()))
    {
        return false;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ExtractionTimer);
    }
    PendingPawn.Reset();
    bExtractionPending = false;
    return Lifecycle->ReturnToHideout(bOperationCompleted);
}
