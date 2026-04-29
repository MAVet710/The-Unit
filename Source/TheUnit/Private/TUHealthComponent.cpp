#include "TUHealthComponent.h"

UTUHealthComponent::UTUHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BodyPartHealth.Add(ETUBodyRegion::Head, 35.f);
	BodyPartHealth.Add(ETUBodyRegion::Chest, 120.f);
	BodyPartHealth.Add(ETUBodyRegion::Stomach, 90.f);
	BodyPartHealth.Add(ETUBodyRegion::LeftArm, 55.f);
	BodyPartHealth.Add(ETUBodyRegion::RightArm, 55.f);
	BodyPartHealth.Add(ETUBodyRegion::LeftLeg, 55.f);
	BodyPartHealth.Add(ETUBodyRegion::RightLeg, 55.f);
}

void UTUHealthComponent::ApplyRegionalDamage(ETUBodyRegion Region, float Damage)
{
	if (IsDead() || Damage <= 0.f) return;
	float* Value = BodyPartHealth.Find(Region);
	if (!Value) return;
	*Value = FMath::Max(0.f, *Value - Damage);
	if ((Region == ETUBodyRegion::Head || Region == ETUBodyRegion::Chest) && *Value <= 0.f)
	{
		OnDeath.Broadcast(GetOwner());
	}
	if (GetTotalHealth() <= 0.f)
	{
		OnDeath.Broadcast(GetOwner());
	}
}

float UTUHealthComponent::GetTotalHealth() const
{
	float Sum = 0.f;
	for (const TPair<ETUBodyRegion, float>& Kvp : BodyPartHealth) Sum += Kvp.Value;
	return Sum;
}

bool UTUHealthComponent::IsDead() const
{
	const float Head = BodyPartHealth.Contains(ETUBodyRegion::Head) ? BodyPartHealth[ETUBodyRegion::Head] : 0.f;
	const float Chest = BodyPartHealth.Contains(ETUBodyRegion::Chest) ? BodyPartHealth[ETUBodyRegion::Chest] : 0.f;
	return Head <= 0.f || Chest <= 0.f || GetTotalHealth() <= 0.f;
}
