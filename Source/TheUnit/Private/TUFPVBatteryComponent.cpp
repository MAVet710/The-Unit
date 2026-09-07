#include "TUFPVBatteryComponent.h"

UTUFPVBatteryComponent::UTUFPVBatteryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    RecalculateLoadedVoltage();
}

void UTUFPVBatteryComponent::BeginPlay()
{
    Super::BeginPlay();
    RecalculateLoadedVoltage();
}

void UTUFPVBatteryComponent::ConsumeCurrent(float CurrentAmps, float DeltaSeconds)
{
    CurrentDrawAmps = FMath::Max(0.0f, CurrentAmps);

    if (DeltaSeconds > 0.0f && CapacityAh > SMALL_NUMBER)
    {
        const float ConsumedAh = (CurrentDrawAmps * DeltaSeconds) / 3600.0f;
        StateOfCharge = FMath::Clamp(StateOfCharge - (ConsumedAh / CapacityAh), 0.0f, 1.0f);
    }

    RecalculateLoadedVoltage();
}

void UTUFPVBatteryComponent::ResetBattery()
{
    StateOfCharge = 1.0f;
    CurrentDrawAmps = 0.0f;
    RecalculateLoadedVoltage();
}

float UTUFPVBatteryComponent::GetThrustScale() const
{
    const float FullPackVoltage = FMath::Max(FullCellVoltage * static_cast<float>(CellCount), SMALL_NUMBER);
    return FMath::Clamp(LoadedVoltage / FullPackVoltage, MinimumThrustScale, 1.0f);
}

bool UTUFPVBatteryComponent::IsLowVoltage() const
{
    return LoadedVoltage <= (LowVoltageCellThreshold * static_cast<float>(CellCount));
}

void UTUFPVBatteryComponent::RecalculateLoadedVoltage()
{
    const float Cells = static_cast<float>(FMath::Max(CellCount, 1));
    const float OpenCircuitCellVoltage = FMath::Lerp(EmptyCellVoltage, FullCellVoltage, StateOfCharge);
    const float OpenCircuitPackVoltage = OpenCircuitCellVoltage * Cells;
    const float SagVoltage = CurrentDrawAmps * FMath::Max(InternalResistanceOhm, 0.0f);
    const float MinimumPackVoltage = EmptyCellVoltage * Cells;

    LoadedVoltage = FMath::Max(MinimumPackVoltage, OpenCircuitPackVoltage - SagVoltage);
}
