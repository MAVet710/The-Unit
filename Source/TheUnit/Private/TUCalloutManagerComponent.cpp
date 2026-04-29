#include "TUCalloutManagerComponent.h"

UTUCalloutManagerComponent::UTUCalloutManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CategoryCooldownSeconds = {
		{ETUCalloutCategory::Movement, 1.0f}, {ETUCalloutCategory::Contact, 2.0f}, {ETUCalloutCategory::WeaponState, 1.0f},
		{ETUCalloutCategory::Injury, 2.0f}, {ETUCalloutCategory::Objective, 1.5f}, {ETUCalloutCategory::Extraction, 1.5f}
	};
}

bool UTUCalloutManagerComponent::TryTriggerCallout(ETUCalloutCategory Category, const FText& Subtitle)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float* Last = LastTriggerTime.Find(Category);
	const float Cooldown = CategoryCooldownSeconds.FindRef(Category);
	if (Last && (Now - *Last) < Cooldown) return false;
	LastTriggerTime.Add(Category, Now);
	OnCalloutTriggered.Broadcast(Category, Subtitle);
	return true;
}
