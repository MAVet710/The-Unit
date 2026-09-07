#include "TUWeaponBuildEditor.h"
#include "TUWeaponBuildRules.h"

namespace
{
const FWeaponPartDefinition* FindPart(
    const TArray<FWeaponPartDefinition>& PartCatalog,
    FName PartId)
{
    return PartCatalog.FindByPredicate([PartId](const FWeaponPartDefinition& Part)
    {
        return Part.PartId == PartId;
    });
}

bool ResolveInstalledDefinitions(
    const FWeaponBuildState& Build,
    const TArray<FWeaponPartDefinition>& PartCatalog,
    TArray<FWeaponPartDefinition>& OutDefinitions,
    FString& OutFailureReason)
{
    OutDefinitions.Reset();
    TSet<FName> SeenPartIds;

    for (const FWeaponInstalledPart& InstalledRef : Build.InstalledParts)
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

        const FWeaponPartDefinition* Definition = FindPart(PartCatalog, InstalledRef.PartId);
        if (!Definition)
        {
            OutFailureReason = TEXT("Build references a part missing from the supplied catalog.");
            return false;
        }
        if (Definition->Slot != InstalledRef.Slot)
        {
            OutFailureReason = TEXT("Installed part slot does not match its immutable definition.");
            return false;
        }
        OutDefinitions.Add(*Definition);
    }

    return true;
}

bool ValidateFinalBuild(
    const FWeaponPlatformDefinition& Platform,
    const FWeaponBuildState& Build,
    const TArray<FWeaponPartDefinition>& PartCatalog,
    FString& OutFailureReason)
{
    if (Build.PlatformId.IsNone() || Build.PlatformId != Platform.PlatformId)
    {
        OutFailureReason = TEXT("Build platform identity does not match the supplied platform.");
        return false;
    }

    TArray<FWeaponPartDefinition> Definitions;
    if (!ResolveInstalledDefinitions(Build, PartCatalog, Definitions, OutFailureReason))
    {
        return false;
    }

    int32 FireControlCount = 0;
    int32 TriggerCount = 0;
    for (const FWeaponPartDefinition& Part : Definitions)
    {
        if (!Platform.SupportedPartSlots.Contains(Part.Slot))
        {
            OutFailureReason = TEXT("Build contains a part category unsupported by the platform.");
            return false;
        }
        if (Part.Slot == ETUWeaponPartSlot::FireControl && ++FireControlCount > 1)
        {
            OutFailureReason = TEXT("Build may contain only one active fire-control module.");
            return false;
        }
        if (Part.Slot == ETUWeaponPartSlot::Trigger && ++TriggerCount > 1)
        {
            OutFailureReason = TEXT("Build may contain only one active trigger.");
            return false;
        }
    }

    const TArray<FName> AvailableTags = FTUWeaponBuildRules::GatherAvailableInterfaceTags(Platform, Definitions);
    for (const FWeaponPartDefinition& Part : Definitions)
    {
        if (!FTUWeaponBuildRules::AreRequiredTagsSatisfied(AvailableTags, Part.RequiredInterfaceTags))
        {
            OutFailureReason = TEXT("Proposed build would leave an installed part without a required interface.");
            return false;
        }
    }

    if (!Build.SelectedAmmoId.IsNone()
        && !FTUWeaponBuildRules::IsAmmoCompatible(Platform, Build.SelectedAmmoId))
    {
        OutFailureReason = TEXT("Proposed build contains ammunition incompatible with the platform.");
        return false;
    }

    OutFailureReason.Reset();
    return true;
}
}

bool FTUWeaponBuildEditor::TryInstallPart(
    const FWeaponPlatformDefinition& Platform,
    const FWeaponPartDefinition& CandidatePart,
    const TArray<FWeaponPartDefinition>& PartCatalog,
    FWeaponBuildState& InOutBuild,
    FString& OutFailureReason)
{
    if (CandidatePart.PartId.IsNone())
    {
        OutFailureReason = TEXT("Cannot install a part with no stable identity.");
        return false;
    }

    const FWeaponPartDefinition* CatalogPart = FindPart(PartCatalog, CandidatePart.PartId);
    if (!CatalogPart)
    {
        OutFailureReason = TEXT("Cannot install a part that is not present in the immutable part catalog.");
        return false;
    }

    if (InOutBuild.InstalledParts.ContainsByPredicate([CatalogPart](const FWeaponInstalledPart& Ref)
        { return Ref.PartId == CatalogPart->PartId; }))
    {
        OutFailureReason = TEXT("The same part identity is already installed.");
        return false;
    }

    FWeaponBuildState Proposed = InOutBuild;
    Proposed.InstalledParts.Add({ CatalogPart->Slot, CatalogPart->PartId });

    if (!ValidateFinalBuild(Platform, Proposed, PartCatalog, OutFailureReason))
    {
        return false;
    }

    InOutBuild = MoveTemp(Proposed);
    return true;
}

bool FTUWeaponBuildEditor::TryRemovePart(
    const FWeaponPlatformDefinition& Platform,
    FName PartId,
    const TArray<FWeaponPartDefinition>& PartCatalog,
    FWeaponBuildState& InOutBuild,
    FString& OutFailureReason)
{
    const int32 ExistingIndex = InOutBuild.InstalledParts.IndexOfByPredicate([PartId](const FWeaponInstalledPart& Ref)
    {
        return Ref.PartId == PartId;
    });
    if (ExistingIndex == INDEX_NONE)
    {
        OutFailureReason = TEXT("Cannot remove a part that is not installed.");
        return false;
    }

    FWeaponBuildState Proposed = InOutBuild;
    Proposed.InstalledParts.RemoveAt(ExistingIndex);
    if (!ValidateFinalBuild(Platform, Proposed, PartCatalog, OutFailureReason))
    {
        return false;
    }

    InOutBuild = MoveTemp(Proposed);
    return true;
}

bool FTUWeaponBuildEditor::TryReplacePart(
    const FWeaponPlatformDefinition& Platform,
    FName ExistingPartId,
    const FWeaponPartDefinition& ReplacementPart,
    const TArray<FWeaponPartDefinition>& PartCatalog,
    FWeaponBuildState& InOutBuild,
    FString& OutFailureReason)
{
    if (ReplacementPart.PartId.IsNone())
    {
        OutFailureReason = TEXT("Cannot install a replacement part with no stable identity.");
        return false;
    }

    const FWeaponPartDefinition* CatalogReplacement = FindPart(PartCatalog, ReplacementPart.PartId);
    if (!CatalogReplacement)
    {
        OutFailureReason = TEXT("Cannot install a replacement part that is not present in the immutable part catalog.");
        return false;
    }

    const int32 ExistingIndex = InOutBuild.InstalledParts.IndexOfByPredicate([ExistingPartId](const FWeaponInstalledPart& Ref)
    {
        return Ref.PartId == ExistingPartId;
    });
    if (ExistingIndex == INDEX_NONE)
    {
        OutFailureReason = TEXT("Cannot replace a part that is not installed.");
        return false;
    }

    if (InOutBuild.InstalledParts.ContainsByPredicate([CatalogReplacement, ExistingPartId](const FWeaponInstalledPart& Ref)
        { return Ref.PartId != ExistingPartId && Ref.PartId == CatalogReplacement->PartId; }))
    {
        OutFailureReason = TEXT("Replacement part identity is already installed elsewhere in the build.");
        return false;
    }

    FWeaponBuildState Proposed = InOutBuild;
    Proposed.InstalledParts[ExistingIndex] = { CatalogReplacement->Slot, CatalogReplacement->PartId };

    if (!ValidateFinalBuild(Platform, Proposed, PartCatalog, OutFailureReason))
    {
        return false;
    }

    InOutBuild = MoveTemp(Proposed);
    return true;
}

bool FTUWeaponBuildEditor::TrySelectAmmo(
    const FWeaponPlatformDefinition& Platform,
    FName AmmoId,
    FWeaponBuildState& InOutBuild,
    FString& OutFailureReason)
{
    if (InOutBuild.PlatformId.IsNone() || InOutBuild.PlatformId != Platform.PlatformId)
    {
        OutFailureReason = TEXT("Build platform identity does not match the supplied platform.");
        return false;
    }

    if (!FTUWeaponBuildRules::IsAmmoCompatible(Platform, AmmoId))
    {
        OutFailureReason = TEXT("Selected ammunition is not compatible with the platform.");
        return false;
    }

    InOutBuild.SelectedAmmoId = AmmoId;
    OutFailureReason.Reset();
    return true;
}
