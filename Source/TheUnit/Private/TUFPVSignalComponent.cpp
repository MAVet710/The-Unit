#include "TUFPVSignalComponent.h"

#include "Engine/World.h"

UTUFPVSignalComponent::UTUFPVSignalComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.10f;
}

void UTUFPVSignalComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshLink();
}

void UTUFPVSignalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshLink();
}

void UTUFPVSignalComponent::SetSignalOrigin(AActor* NewOrigin)
{
    SignalOrigin = NewOrigin;
    RefreshLink();
}

float UTUFPVSignalComponent::ComputeDistanceQuality(float Distance, float Range) const
{
    if (Range <= SMALL_NUMBER)
    {
        return 0.0f;
    }

    const float Linear = FMath::Clamp(1.0f - (Distance / Range), 0.0f, 1.0f);
    return Linear * Linear * (3.0f - (2.0f * Linear));
}

void UTUFPVSignalComponent::RefreshLink()
{
    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();

    if (!OwnerActor || !World || !IsValid(SignalOrigin))
    {
        VideoQuality = 1.0f;
        ControlQuality = 1.0f;
        DistanceMeters = 0.0f;
        bObstructed = false;
        VideoLatencyMs = VideoLinkType == ETUFPVVideoLinkType::Analog ? 8.0f : 28.0f;
        return;
    }

    const FVector Start = SignalOrigin->GetActorLocation();
    const FVector End = OwnerActor->GetActorLocation();
    DistanceMeters = FVector::Distance(Start, End) / 100.0f;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FPVSignalTrace), false);
    Params.AddIgnoredActor(OwnerActor);
    Params.AddIgnoredActor(SignalOrigin);

    bObstructed = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    VideoQuality = ComputeDistanceQuality(DistanceMeters, VideoClearRangeMeters);
    ControlQuality = ComputeDistanceQuality(DistanceMeters, ControlClearRangeMeters);

    if (bObstructed)
    {
        VideoQuality *= FMath::Clamp(ObstructionVideoMultiplier, 0.0f, 1.0f);
        ControlQuality *= FMath::Clamp(ObstructionControlMultiplier, 0.0f, 1.0f);
    }

    if (VideoLinkType == ETUFPVVideoLinkType::Analog)
    {
        // Analog degrades primarily through image quality rather than buffering.
        VideoLatencyMs = FMath::Lerp(8.0f, 12.0f, 1.0f - VideoQuality);
    }
    else
    {
        // Digital links retain a clean image longer but can add delay near the edge.
        VideoLatencyMs = FMath::Lerp(28.0f, 85.0f, 1.0f - VideoQuality);
    }
}
