#include "TUHideoutProgressionComponent.h"

UTUHideoutProgressionComponent::UTUHideoutProgressionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    const ETUHideoutModuleType Defaults[] = {
        ETUHideoutModuleType::Power,
        ETUHideoutModuleType::Communications,
        ETUHideoutModuleType::Medical,
        ETUHideoutModuleType::Storage,
        ETUHideoutModuleType::ArmorySupport,
        ETUHideoutModuleType::GearMaintenance,
        ETUHideoutModuleType::Planning,
        ETUHideoutModuleType::RangeSupport
    };

    for (ETUHideoutModuleType Type : Defaults)
    {
        FTUHideoutModuleState State;
        State.Type = Type;
        State.Level = 0;
        Modules.Add(State);
    }

    SetModuleLevel(ETUHideoutModuleType::Power, 1);
    SetModuleLevel(ETUHideoutModuleType::Communications, 1);
    SetModuleLevel(ETUHideoutModuleType::Storage, 1);
    SetModuleLevel(ETUHideoutModuleType::ArmorySupport, 1);
    SetModuleLevel(ETUHideoutModuleType::GearMaintenance, 1);
    SetModuleLevel(ETUHideoutModuleType::Planning, 1);
    SetModuleLevel(ETUHideoutModuleType::RangeSupport, 1);
}

int32 UTUHideoutProgressionComponent::FindModuleIndex(ETUHideoutModuleType Type) const
{
    for (int32 Index = 0; Index < Modules.Num(); ++Index)
    {
        if (Modules[Index].Type == Type)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

int32 UTUHideoutProgressionComponent::GetModuleLevel(ETUHideoutModuleType Type) const
{
    const int32 Index = FindModuleIndex(Type);
    return Index == INDEX_NONE ? 0 : Modules[Index].Level;
}

bool UTUHideoutProgressionComponent::SetModuleLevel(ETUHideoutModuleType Type, int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel, 0, 3);
    const int32 Index = FindModuleIndex(Type);
    if (Index == INDEX_NONE)
    {
        FTUHideoutModuleState State;
        State.Type = Type;
        State.Level = Clamped;
        Modules.Add(State);
        return true;
    }

    if (Modules[Index].Level == Clamped)
    {
        return false;
    }

    Modules[Index].Level = Clamped;
    return true;
}

bool UTUHideoutProgressionComponent::UpgradeModule(ETUHideoutModuleType Type)
{
    const int32 Current = GetModuleLevel(Type);
    if (Current >= 3)
    {
        return false;
    }
    return SetModuleLevel(Type, Current + 1);
}
