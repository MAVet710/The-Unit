#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUFPVBatteryComponent.generated.h"

/** Lightweight LiPo model for the FPV flight simulation. */
UCLASS(ClassGroup=(FPV), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUFPVBatteryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUFPVBatteryComponent();

    /** Advance discharge using estimated pack current draw. */
    void ConsumeCurrent(float CurrentAmps, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="FPV|Battery")
    void ResetBattery();

    UFUNCTION(BlueprintPure, Category="FPV|Battery")
    float GetVoltage() const { return LoadedVoltage; }

    UFUNCTION(BlueprintPure, Category="FPV|Battery")
    float GetCurrentDrawAmps() const { return CurrentDrawAmps; }

    UFUNCTION(BlueprintPure, Category="FPV|Battery")
    float GetStateOfCharge() const { return StateOfCharge; }

    UFUNCTION(BlueprintPure, Category="FPV|Battery")
    float GetBatteryPercent() const { return StateOfCharge * 100.0f; }

    UFUNCTION(BlueprintPure, Category="FPV|Battery")
    float GetThrustScale() const;

    UFUNCTION(BlueprintPure, Category="FPV|Battery")
    bool IsLowVoltage() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="1", ClampMax="12"))
    int32 CellCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="0.1"))
    float CapacityAh = 1.30f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="3.5", ClampMax="4.5"))
    float FullCellVoltage = 4.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="2.8", ClampMax="3.8"))
    float EmptyCellVoltage = 3.30f;

    /** Whole-pack effective internal resistance, used to produce voltage sag. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="0.0"))
    float InternalResistanceOhm = 0.028f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="2.8", ClampMax="4.0"))
    float LowVoltageCellThreshold = 3.50f;

    /** Prevents the simplified model from removing all thrust at an empty pack. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPV|Battery", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinimumThrustScale = 0.68f;

private:
    void RecalculateLoadedVoltage();

    UPROPERTY(VisibleAnywhere, Category="FPV|Battery")
    float StateOfCharge = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="FPV|Battery")
    float CurrentDrawAmps = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="FPV|Battery")
    float LoadedVoltage = 25.2f;
};
