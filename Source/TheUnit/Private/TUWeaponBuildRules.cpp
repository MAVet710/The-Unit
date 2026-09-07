#include "TUWeaponBuildRules.h"

TArray<FName> FTUWeaponBuildRules::GatherAvailableInterfaceTags(
    const FWeaponPlatformDefinition& Platform,
    const TArray<FWeaponPartDefinition>& InstalledParts)
{
    TArray<FName> Result;

    for (const FName Tag : Platform.InterfaceTags)
    {
        if (!Tag.IsNone())
        {
            Result.AddUnique(Tag);
        }
    }

    for (const FWeaponPartDefinition& Part : InstalledParts)
    {
        for (const FName Tag : Part.ProvidedInterfaceTags)
        {
            if (!Tag.IsNone())
            {
                Result.AddUnique(Tag);
            }
        }
    }

    return Result;
}

bool FTUWeaponBuildRules::AreRequiredTagsSatisfied(
    const TArray<FName>& AvailableTags,
    const TArray<FName>& RequiredTags)
{
    for (const FName RequiredTag : RequiredTags)
    {
        if (!RequiredTag.IsNone() && !AvailableTags.Contains(RequiredTag))
        {
            return false;
        }
    }

    return true;
}

bool FTUWeaponBuildRules::IsPartCompatible(
    const FWeaponPlatformDefinition& Platform,
    const FWeaponPartDefinition& CandidatePart,
    const TArray<FWeaponPartDefinition>& InstalledParts,
    FString* OutFailureReason)
{
    if (!Platform.SupportedPartSlots.Contains(CandidatePart.Slot))
    {
        if (OutFailureReason)
        {
            *OutFailureReason = TEXT("Platform does not support the candidate part slot.");
        }
        return false;
    }

    const TArray<FName> AvailableTags = GatherAvailableInterfaceTags(Platform, InstalledParts);
    if (!AreRequiredTagsSatisfied(AvailableTags, CandidatePart.RequiredInterfaceTags))
    {
        if (OutFailureReason)
        {
            *OutFailureReason = TEXT("Candidate part requires an interface tag not provided by the current build.");
        }
        return false;
    }

    if (OutFailureReason)
    {
        OutFailureReason->Reset();
    }
    return true;
}

bool FTUWeaponBuildRules::IsAmmoCompatible(
    const FWeaponPlatformDefinition& Platform,
    FName AmmoId)
{
    return !AmmoId.IsNone() && Platform.CompatibleAmmoIds.Contains(AmmoId);
}

FWeaponDefinition FTUWeaponBuildRules::DeriveWeaponDefinition(
    const FWeaponDefinition& BaseDefinition,
    const TArray<FWeaponPartDefinition>& InstalledParts)
{
    FWeaponDefinition Result = BaseDefinition;

    for (const FWeaponPartDefinition& Part : InstalledParts)
    {
        Result.RecoilPitch *= Part.RecoilPitchMultiplier;
        Result.RecoilYaw *= Part.RecoilYawMultiplier;
        Result.HipSpread *= Part.HipSpreadMultiplier;
        Result.ADSSpread *= Part.ADSSpreadMultiplier;
        Result.FireRateRPM *= Part.FireRateRPMMultiplier;
    }

    Result.RecoilPitch = FMath::Max(0.0f, Result.RecoilPitch);
    Result.RecoilYaw = FMath::Max(0.0f, Result.RecoilYaw);
    Result.HipSpread = FMath::Max(0.0f, Result.HipSpread);
    Result.ADSSpread = FMath::Max(0.0f, Result.ADSSpread);
    Result.FireRateRPM = FMath::Max(0.0f, Result.FireRateRPM);

    return Result;
}
