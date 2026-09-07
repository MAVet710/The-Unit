#include "TUWeaponDefinitionCatalog.h"
#include "TUWeaponBuildResolver.h"

namespace
{
template <typename TDefinition, typename TPredicate>
const TDefinition* FindDefinition(const TArray<TDefinition>& Definitions, TPredicate&& Predicate)
{
    return Definitions.FindByPredicate(Forward<TPredicate>(Predicate));
}

void ValidateId(FName Id, const TCHAR* Label, TSet<FName>& SeenIds, TArray<FString>& OutErrors)
{
    if (Id.IsNone())
    {
        OutErrors.Add(FString::Printf(TEXT("%s has no stable ID."), Label));
        return;
    }

    if (SeenIds.Contains(Id))
    {
        OutErrors.Add(FString::Printf(TEXT("Duplicate %s ID: %s."), Label, *Id.ToString()));
        return;
    }

    SeenIds.Add(Id);
}

void ValidateTagArray(
    const TArray<FName>& Tags,
    const FString& OwnerLabel,
    const TCHAR* FieldLabel,
    TArray<FString>& OutErrors)
{
    TSet<FName> SeenTags;
    for (const FName Tag : Tags)
    {
        if (Tag.IsNone())
        {
            OutErrors.Add(FString::Printf(TEXT("%s contains an empty %s tag."), *OwnerLabel, FieldLabel));
            continue;
        }
        if (SeenTags.Contains(Tag))
        {
            OutErrors.Add(FString::Printf(
                TEXT("%s contains duplicate %s tag %s."),
                *OwnerLabel,
                FieldLabel,
                *Tag.ToString()));
            continue;
        }
        SeenTags.Add(Tag);
    }
}
}

bool UTUWeaponDefinitionCatalog::GetPlatformDefinition(
    FName PlatformId,
    FWeaponPlatformDefinition& OutDefinition) const
{
    const FWeaponPlatformDefinition* Found = FindDefinition(
        Platforms, [PlatformId](const FWeaponPlatformDefinition& Definition)
        {
            return Definition.PlatformId == PlatformId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetBaseWeaponDefinition(
    FName WeaponId,
    FWeaponDefinition& OutDefinition) const
{
    const FWeaponDefinition* Found = FindDefinition(
        BaseWeaponDefinitions, [WeaponId](const FWeaponDefinition& Definition)
        {
            return Definition.WeaponId == WeaponId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetPartDefinition(
    FName PartId,
    FWeaponPartDefinition& OutDefinition) const
{
    const FWeaponPartDefinition* Found = FindDefinition(
        Parts, [PartId](const FWeaponPartDefinition& Definition)
        {
            return Definition.PartId == PartId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetFireControlDefinition(
    FName FireControlId,
    FFireControlModuleDefinition& OutDefinition) const
{
    const FFireControlModuleDefinition* Found = FindDefinition(
        FireControlModules, [FireControlId](const FFireControlModuleDefinition& Definition)
        {
            return Definition.FireControlId == FireControlId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetTriggerDefinition(
    FName TriggerId,
    FTriggerDefinition& OutDefinition) const
{
    const FTriggerDefinition* Found = FindDefinition(
        Triggers, [TriggerId](const FTriggerDefinition& Definition)
        {
            return Definition.TriggerId == TriggerId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::GetAmmoDefinition(
    FName AmmoId,
    FAmmoDefinition& OutDefinition) const
{
    const FAmmoDefinition* Found = FindDefinition(
        Ammunition, [AmmoId](const FAmmoDefinition& Definition)
        {
            return Definition.AmmoId == AmmoId;
        });
    if (!Found)
    {
        return false;
    }
    OutDefinition = *Found;
    return true;
}

bool UTUWeaponDefinitionCatalog::ValidateCatalog(TArray<FString>& OutErrors) const
{
    OutErrors.Reset();

    TSet<FName> PlatformIds;
    for (const FWeaponPlatformDefinition& Platform : Platforms)
    {
        ValidateId(Platform.PlatformId, TEXT("platform"), PlatformIds, OutErrors);
        const FString OwnerLabel = FString::Printf(TEXT("Platform %s"), *Platform.PlatformId.ToString());
        ValidateTagArray(Platform.InterfaceTags, OwnerLabel, TEXT("interface"), OutErrors);

        if (Platform.BaseWeaponDefinitionId.IsNone())
        {
            OutErrors.Add(FString::Printf(TEXT("%s has no base weapon definition ID."), *OwnerLabel));
        }
        else
        {
            FWeaponDefinition BaseDefinition;
            if (!GetBaseWeaponDefinition(Platform.BaseWeaponDefinitionId, BaseDefinition))
            {
                OutErrors.Add(FString::Printf(
                    TEXT("%s references missing base weapon definition %s."),
                    *OwnerLabel,
                    *Platform.BaseWeaponDefinitionId.ToString()));
            }
        }

        TArray<ETUWeaponPartSlot> SeenSlots;
        for (const ETUWeaponPartSlot Slot : Platform.SupportedPartSlots)
        {
            if (SeenSlots.Contains(Slot))
            {
                OutErrors.Add(FString::Printf(TEXT("%s contains a duplicate supported part slot."), *OwnerLabel));
            }
            else
            {
                SeenSlots.Add(Slot);
            }
        }

        TSet<FName> SeenAmmoIds;
        for (const FName AmmoId : Platform.CompatibleAmmoIds)
        {
            if (AmmoId.IsNone())
            {
                OutErrors.Add(FString::Printf(TEXT("%s contains an empty compatible ammo ID."), *OwnerLabel));
                continue;
            }
            if (SeenAmmoIds.Contains(AmmoId))
            {
                OutErrors.Add(FString::Printf(
                    TEXT("%s contains duplicate compatible ammo ID %s."),
                    *OwnerLabel,
                    *AmmoId.ToString()));
                continue;
            }
            SeenAmmoIds.Add(AmmoId);

            FAmmoDefinition AmmoDefinition;
            if (!GetAmmoDefinition(AmmoId, AmmoDefinition))
            {
                OutErrors.Add(FString::Printf(
                    TEXT("%s references missing ammunition %s."),
                    *OwnerLabel,
                    *AmmoId.ToString()));
            }
        }
    }

    TSet<FName> WeaponIds;
    for (const FWeaponDefinition& Definition : BaseWeaponDefinitions)
    {
        ValidateId(Definition.WeaponId, TEXT("base weapon"), WeaponIds, OutErrors);
    }

    TSet<FName> AmmoIds;
    for (const FAmmoDefinition& Definition : Ammunition)
    {
        ValidateId(Definition.AmmoId, TEXT("ammunition"), AmmoIds, OutErrors);
    }

    TSet<FName> FireControlIds;
    for (const FFireControlModuleDefinition& Definition : FireControlModules)
    {
        ValidateId(Definition.FireControlId, TEXT("fire-control"), FireControlIds, OutErrors);
        const FString OwnerLabel = FString::Printf(TEXT("Fire-control %s"), *Definition.FireControlId.ToString());

        if (Definition.SupportedFireModes.Num() == 0)
        {
            OutErrors.Add(FString::Printf(TEXT("%s exposes no supported fire mode."), *OwnerLabel));
        }
        if (Definition.SupportedFireModes.Contains(ETUFireMode::Burst) && Definition.BurstCount < 1)
        {
            OutErrors.Add(FString::Printf(TEXT("%s supports burst but has an invalid burst count."), *OwnerLabel));
        }
        if (Definition.TriggerResponseMultiplier < 0.0f
            || Definition.ResetResponseMultiplier < 0.0f
            || Definition.SemiAutoResetDelaySeconds < 0.0f)
        {
            OutErrors.Add(FString::Printf(TEXT("%s has a negative legacy trigger-response value."), *OwnerLabel));
        }
    }

    TSet<FName> TriggerIds;
    for (const FTriggerDefinition& Definition : Triggers)
    {
        ValidateId(Definition.TriggerId, TEXT("trigger"), TriggerIds, OutErrors);
        const FString OwnerLabel = FString::Printf(TEXT("Trigger %s"), *Definition.TriggerId.ToString());
        if (Definition.TriggerResponseMultiplier < 0.0f
            || Definition.ResetResponseMultiplier < 0.0f
            || Definition.SemiAutoResetDelaySeconds < 0.0f)
        {
            OutErrors.Add(FString::Printf(TEXT("%s has a negative response value."), *OwnerLabel));
        }
    }

    TSet<FName> PartIds;
    for (const FWeaponPartDefinition& Part : Parts)
    {
        ValidateId(Part.PartId, TEXT("part"), PartIds, OutErrors);
        const FString OwnerLabel = FString::Printf(TEXT("Part %s"), *Part.PartId.ToString());
        ValidateTagArray(Part.RequiredInterfaceTags, OwnerLabel, TEXT("required interface"), OutErrors);
        ValidateTagArray(Part.ProvidedInterfaceTags, OwnerLabel, TEXT("provided interface"), OutErrors);

        if (Part.RecoilPitchMultiplier < 0.0f
            || Part.RecoilYawMultiplier < 0.0f
            || Part.HipSpreadMultiplier < 0.0f
            || Part.ADSSpreadMultiplier < 0.0f
            || Part.FireRateRPMMultiplier < 0.0f)
        {
            OutErrors.Add(FString::Printf(TEXT("%s has a negative gameplay multiplier."), *OwnerLabel));
        }

        if (Part.Slot == ETUWeaponPartSlot::FireControl)
        {
            if (Part.FireControlDefinitionId.IsNone())
            {
                OutErrors.Add(FString::Printf(TEXT("%s has no fire-control definition ID."), *OwnerLabel));
            }
            else
            {
                FFireControlModuleDefinition Definition;
                if (!GetFireControlDefinition(Part.FireControlDefinitionId, Definition))
                {
                    OutErrors.Add(FString::Printf(
                        TEXT("%s references missing fire-control definition %s."),
                        *OwnerLabel,
                        *Part.FireControlDefinitionId.ToString()));
                }
            }
        }
        else if (!Part.FireControlDefinitionId.IsNone())
        {
            OutErrors.Add(FString::Printf(TEXT("%s is not a FireControl part but references fire-control behavior."), *OwnerLabel));
        }

        if (Part.Slot == ETUWeaponPartSlot::Trigger)
        {
            if (Part.TriggerDefinitionId.IsNone())
            {
                OutErrors.Add(FString::Printf(TEXT("%s has no trigger definition ID."), *OwnerLabel));
            }
            else
            {
                FTriggerDefinition Definition;
                if (!GetTriggerDefinition(Part.TriggerDefinitionId, Definition))
                {
                    OutErrors.Add(FString::Printf(
                        TEXT("%s references missing trigger definition %s."),
                        *OwnerLabel,
                        *Part.TriggerDefinitionId.ToString()));
                }
            }
        }
        else if (!Part.TriggerDefinitionId.IsNone())
        {
            OutErrors.Add(FString::Printf(TEXT("%s is not a Trigger part but references trigger behavior."), *OwnerLabel));
        }
    }

    return OutErrors.Num() == 0;
}

bool UTUWeaponDefinitionCatalog::ResolveWeaponBuild(
    const FWeaponBuildState& BuildState,
    FTUResolvedWeaponBuild& OutResolvedBuild,
    FString& OutFailureReason) const
{
    OutResolvedBuild = FTUResolvedWeaponBuild();
    OutFailureReason.Reset();

    FWeaponPlatformDefinition Platform;
    if (!GetPlatformDefinition(BuildState.PlatformId, Platform))
    {
        OutFailureReason = TEXT("Weapon build references a platform missing from the definition catalog.");
        return false;
    }

    if (Platform.BaseWeaponDefinitionId.IsNone())
    {
        OutFailureReason = TEXT("Weapon platform does not reference a base weapon definition.");
        return false;
    }

    FWeaponDefinition BaseDefinition;
    if (!GetBaseWeaponDefinition(Platform.BaseWeaponDefinitionId, BaseDefinition))
    {
        OutFailureReason = TEXT("Weapon platform references a base weapon definition missing from the catalog.");
        return false;
    }

    FAmmoDefinition SelectedAmmoDefinition;
    const bool bHasSelectedAmmo = !BuildState.SelectedAmmoId.IsNone();
    if (bHasSelectedAmmo && !GetAmmoDefinition(BuildState.SelectedAmmoId, SelectedAmmoDefinition))
    {
        OutFailureReason = TEXT("Weapon build references ammunition missing from the definition catalog.");
        return false;
    }

    FTUResolvedWeaponBuild ResolvedBuild;
    if (!FTUWeaponBuildResolver::ResolveBuild(
        Platform,
        BaseDefinition,
        BuildState,
        Parts,
        FireControlModules,
        ResolvedBuild,
        OutFailureReason))
    {
        return false;
    }

    if (ResolvedBuild.bHasTriggerPart)
    {
        FTriggerDefinition TriggerDefinition;
        if (!GetTriggerDefinition(ResolvedBuild.TriggerDefinitionId, TriggerDefinition))
        {
            OutFailureReason = TEXT("Trigger part references a trigger definition missing from the catalog.");
            return false;
        }
        ResolvedBuild.bHasTriggerDefinition = true;
        ResolvedBuild.TriggerDefinition = TriggerDefinition;
    }

    if (bHasSelectedAmmo)
    {
        ResolvedBuild.bHasAmmoDefinition = true;
        ResolvedBuild.AmmoDefinition = SelectedAmmoDefinition;
    }

    OutResolvedBuild = MoveTemp(ResolvedBuild);
    return true;
}
