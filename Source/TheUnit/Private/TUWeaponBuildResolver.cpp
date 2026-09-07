#include "TUWeaponBuildResolver.h"
#include "TUWeaponBuildRules.h"

namespace
{
const FWeaponPartDefinition* FindPartDefinition(
    const TArray<FWeaponPartDefinition>& PartCatalog,
    FName PartId)
{
    return PartCatalog.FindByPredicate([PartId](const FWeaponPartDefinition& Part)
    {
        return Part.PartId == PartId;
    });
}

const FFireControlModuleDefinition* FindFireControlDefinition(
    const TArray<FFireControlModuleDefinition>& FireControlCatalog,
    FName FireControlId)
{
    return FireControlCatalog.FindByPredicate([FireControlId](const FFireControlModuleDefinition& Definition)
    {
        return Definition.FireControlId == FireControlId;
    });
}
}

bool FTUWeaponBuildResolver::ResolveBuild(
    const FWeaponPlatformDefinition& Platform,
    const FWeaponDefinition& BaseWeaponDefinition,
    const FWeaponBuildState& BuildState,
    const TArray<FWeaponPartDefinition>& PartCatalog,
    const TArray<FFireControlModuleDefinition>& FireControlCatalog,
    FTUResolvedWeaponBuild& OutResolvedBuild,
    FString& OutFailureReason)
{
    OutResolvedBuild = FTUResolvedWeaponBuild();
    OutFailureReason.Reset();

    if (BuildState.PlatformId.IsNone() || BuildState.PlatformId != Platform.PlatformId)
    {
        OutFailureReason = TEXT("Build platform identity does not match the supplied platform definition.");
        return false;
    }

    if (!BuildState.SelectedAmmoId.IsNone()
        && !FTUWeaponBuildRules::IsAmmoCompatible(Platform, BuildState.SelectedAmmoId))
    {
        OutFailureReason = TEXT("Selected ammunition is not compatible with the platform.");
        return false;
    }

    TSet<FName> SeenPartIds;
    for (const FWeaponInstalledPart& InstalledRef : BuildState.InstalledParts)
    {
        if (InstalledRef.PartId.IsNone())
        {
            OutFailureReason = TEXT("Build contains an installed part with no identity.");
            return false;
        }

        if (SeenPartIds.Contains(InstalledRef.PartId))
        {
            OutFailureReason = TEXT("Build contains the same part identity more than once.");
            return false;
        }
        SeenPartIds.Add(InstalledRef.PartId);

        const FWeaponPartDefinition* PartDefinition = FindPartDefinition(PartCatalog, InstalledRef.PartId);
        if (!PartDefinition)
        {
            OutFailureReason = TEXT("Build references a part that is missing from the supplied catalog.");
            return false;
        }

        if (PartDefinition->Slot != InstalledRef.Slot)
        {
            OutFailureReason = TEXT("Installed part slot does not match its immutable part definition.");
            return false;
        }

        if (!Platform.SupportedPartSlots.Contains(PartDefinition->Slot))
        {
            OutFailureReason = TEXT("Build contains a part category unsupported by the platform.");
            return false;
        }

        OutResolvedBuild.InstalledPartDefinitions.Add(*PartDefinition);
    }

    const TArray<FName> AvailableTags = FTUWeaponBuildRules::GatherAvailableInterfaceTags(
        Platform, OutResolvedBuild.InstalledPartDefinitions);

    int32 FireControlPartCount = 0;
    int32 TriggerPartCount = 0;
    for (const FWeaponPartDefinition& Part : OutResolvedBuild.InstalledPartDefinitions)
    {
        if (!FTUWeaponBuildRules::AreRequiredTagsSatisfied(AvailableTags, Part.RequiredInterfaceTags))
        {
            OutFailureReason = TEXT("Resolved build does not satisfy one or more part interface requirements.");
            return false;
        }

        if (Part.Slot == ETUWeaponPartSlot::FireControl)
        {
            ++FireControlPartCount;
            if (FireControlPartCount > 1)
            {
                OutFailureReason = TEXT("A resolved build may contain only one active fire-control module.");
                return false;
            }

            if (Part.FireControlDefinitionId.IsNone())
            {
                OutFailureReason = TEXT("Fire-control part does not reference a fire-control behavior definition.");
                return false;
            }

            const FFireControlModuleDefinition* FireControl = FindFireControlDefinition(
                FireControlCatalog, Part.FireControlDefinitionId);
            if (!FireControl)
            {
                OutFailureReason = TEXT("Fire-control behavior definition is missing from the supplied catalog.");
                return false;
            }

            OutResolvedBuild.bHasFireControl = true;
            OutResolvedBuild.FireControlDefinition = *FireControl;
        }
        else if (Part.Slot == ETUWeaponPartSlot::Trigger)
        {
            ++TriggerPartCount;
            if (TriggerPartCount > 1)
            {
                OutFailureReason = TEXT("A resolved build may contain only one active trigger.");
                return false;
            }

            if (Part.TriggerDefinitionId.IsNone())
            {
                OutFailureReason = TEXT("Trigger part does not reference a trigger behavior definition.");
                return false;
            }

            OutResolvedBuild.bHasTriggerPart = true;
            OutResolvedBuild.TriggerDefinitionId = Part.TriggerDefinitionId;
        }
    }

    OutResolvedBuild.DerivedWeaponDefinition = FTUWeaponBuildRules::DeriveWeaponDefinition(
        BaseWeaponDefinition, OutResolvedBuild.InstalledPartDefinitions);

    return true;
}
