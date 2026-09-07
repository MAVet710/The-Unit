#include "TUArmorProtectionComponent.h"

#include "TUEquipmentDefinition.h"
#include "TUOperatorEquipmentComponent.h"

UTUArmorProtectionComponent::UTUArmorProtectionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTUArmorProtectionComponent::InitializeEquipment(UTUOperatorEquipmentComponent* InEquipmentComponent)
{
    if (EquipmentComponent != InEquipmentComponent)
    {
        EquipmentComponent = InEquipmentComponent;
        ResetArmorState();
    }
}

FTUArmorHitResult UTUArmorProtectionComponent::ResolveBallisticHit(
    ETUBodyRegion Region,
    float IncomingDamage,
    float Penetration,
    float ArmorDamage,
    float CoverageRoll01)
{
    FTUArmorHitResult Result;
    Result.IncomingDamage = FMath::Max(0.0f, IncomingDamage);
    Result.FinalDamage = Result.IncomingDamage;

    if (Result.IncomingDamage <= 0.0f || !EquipmentComponent)
    {
        return Result;
    }

    ETUEquipmentSlot ProtectiveSlot = ETUEquipmentSlot::Accessory;
    const UTUEquipmentDefinition* Definition = FindBestProtectiveItem(
        Region,
        FMath::Clamp(CoverageRoll01, 0.0f, 1.0f),
        ProtectiveSlot);

    if (!Definition)
    {
        return Result;
    }

    Result.bArmorPresent = true;
    Result.bCoverageHit = true;
    Result.ArmorSlot = ProtectiveSlot;
    Result.ArmorItemId = MakeItemKey(Definition);

    FTUArmorRuntimeState& State = GetOrCreateRuntimeState(ProtectiveSlot, Definition);
    Result.DurabilityBefore = State.Durability;

    const bool bArmorConfigured = Definition->PenetrationResistance > 0.0f && Definition->MaxArmorDurability > 0.0f;
    if (!bArmorConfigured || State.Durability <= 0.0f)
    {
        Result.bArmorDepleted = State.Durability <= 0.0f;
        Result.bPenetrated = true;
        Result.DurabilityAfter = State.Durability;
        return Result;
    }

    const float RequestedArmorDamage = FMath::Max(0.0f, ArmorDamage);
    Result.ArmorDamageApplied = FMath::Min(State.Durability, RequestedArmorDamage);

    const bool bStopsProjectile = FMath::Max(0.0f, Penetration) <= Definition->PenetrationResistance;
    if (bStopsProjectile)
    {
        Result.bStopped = true;
        Result.FinalDamage = Result.IncomingDamage * FMath::Clamp(Definition->StoppedRoundDamageMultiplier, 0.0f, 1.0f);
    }
    else
    {
        Result.bPenetrated = true;
    }

    State.Durability = FMath::Max(0.0f, State.Durability - Result.ArmorDamageApplied);
    Result.DurabilityAfter = State.Durability;
    Result.bArmorDepleted = State.Durability <= 0.0f;

    return Result;
}

float UTUArmorProtectionComponent::GetArmorDurability(ETUEquipmentSlot Slot) const
{
    if (const FTUArmorRuntimeState* State = RuntimeArmorState.Find(Slot))
    {
        return State->Durability;
    }

    return 0.0f;
}

void UTUArmorProtectionComponent::ResetArmorState()
{
    RuntimeArmorState.Empty();
}

const UTUEquipmentDefinition* UTUArmorProtectionComponent::FindBestProtectiveItem(
    ETUBodyRegion Region,
    float CoverageRoll01,
    ETUEquipmentSlot& OutSlot) const
{
    if (!EquipmentComponent)
    {
        return nullptr;
    }

    static constexpr ETUEquipmentSlot CandidateSlots[] = {
        ETUEquipmentSlot::Headwear,
        ETUEquipmentSlot::Headset,
        ETUEquipmentSlot::Eyewear,
        ETUEquipmentSlot::Facewear,
        ETUEquipmentSlot::NVG,
        ETUEquipmentSlot::TorsoArmor,
        ETUEquipmentSlot::ChestRig,
        ETUEquipmentSlot::Backpack,
        ETUEquipmentSlot::Belt,
        ETUEquipmentSlot::LeftHip,
        ETUEquipmentSlot::RightHip,
        ETUEquipmentSlot::Gloves,
        ETUEquipmentSlot::KneePads,
        ETUEquipmentSlot::Footwear,
        ETUEquipmentSlot::Accessory
    };

    const UTUEquipmentDefinition* BestDefinition = nullptr;
    float BestResistance = -1.0f;

    for (const ETUEquipmentSlot Slot : CandidateSlots)
    {
        const UTUEquipmentDefinition* Definition = EquipmentComponent->GetEquippedItem(Slot);
        if (!Definition || !Definition->bProvidesBallisticProtection)
        {
            continue;
        }

        if (!Definition->ProtectedRegions.Contains(Region))
        {
            continue;
        }

        if (CoverageRoll01 > FMath::Clamp(Definition->RegionalCoverageFraction, 0.0f, 1.0f))
        {
            continue;
        }

        if (!BestDefinition || Definition->PenetrationResistance > BestResistance)
        {
            BestDefinition = Definition;
            BestResistance = Definition->PenetrationResistance;
            OutSlot = Slot;
        }
    }

    return BestDefinition;
}

FName UTUArmorProtectionComponent::MakeItemKey(const UTUEquipmentDefinition* Definition) const
{
    if (!Definition)
    {
        return NAME_None;
    }

    return Definition->ItemId.IsNone() ? Definition->GetFName() : Definition->ItemId;
}

FTUArmorRuntimeState& UTUArmorProtectionComponent::GetOrCreateRuntimeState(
    ETUEquipmentSlot Slot,
    const UTUEquipmentDefinition* Definition)
{
    FTUArmorRuntimeState& State = RuntimeArmorState.FindOrAdd(Slot);
    const FName DesiredItemKey = MakeItemKey(Definition);

    if (State.ItemKey != DesiredItemKey)
    {
        State.ItemKey = DesiredItemKey;
        State.Durability = Definition ? FMath::Max(0.0f, Definition->MaxArmorDurability) : 0.0f;
    }

    return State;
}
