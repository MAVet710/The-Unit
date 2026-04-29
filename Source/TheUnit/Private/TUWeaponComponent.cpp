#include "TUWeaponComponent.h"

bool UTUWeaponComponent::FireSemiAuto()
{
	if (MagazineState.bRoundChambered)
	{
		MagazineState.bRoundChambered = false;
		if (MagazineState.RoundsInMagazine > 0)
		{
			MagazineState.RoundsInMagazine -= 1;
			MagazineState.bRoundChambered = true;
		}
		return true;
	}
	if (MagazineState.RoundsInMagazine > 0)
	{
		MagazineState.RoundsInMagazine -= 1;
		MagazineState.bRoundChambered = true;
		return true;
	}
	return false;
}

bool UTUWeaponComponent::Reload()
{
	const int32 Needed = MagazineState.Capacity - MagazineState.RoundsInMagazine;
	if (Needed <= 0 || AmmoReserve <= 0) return false;
	const int32 ToLoad = FMath::Min(Needed, AmmoReserve);
	MagazineState.RoundsInMagazine += ToLoad;
	AmmoReserve -= ToLoad;
	if (!MagazineState.bRoundChambered && MagazineState.RoundsInMagazine > 0)
	{
		MagazineState.RoundsInMagazine -= 1;
		MagazineState.bRoundChambered = true;
	}
	return true;
}
