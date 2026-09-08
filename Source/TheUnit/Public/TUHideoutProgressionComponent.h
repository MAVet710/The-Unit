#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUHideoutProgressionComponent.generated.h"

UENUM(BlueprintType)
enum class ETUHideoutModuleType : uint8
{
    Power UMETA(DisplayName="Power"),
    Communications UMETA(DisplayName="Communications"),
    Medical UMETA(DisplayName="Medical"),
    Storage UMETA(DisplayName="Storage"),
    ArmorySupport UMETA(DisplayName="Armory Support"),
    GearMaintenance UMETA(DisplayName="Gear Maintenance"),
    Planning UMETA(DisplayName="Planning"),
    RangeSupport UMETA(DisplayName="Range Support")
};

USTRUCT(BlueprintType)
struct FTUHideoutModuleState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETUHideoutModuleType Type = ETUHideoutModuleType::Power;

    /** 0 = unavailable/unbuilt, 1..3 = increasingly developed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="3"))
    int32 Level = 0;
};

/**
 * Lightweight progression state for the pre-mission hideout.
 * Resource costs/unlocks are deliberately kept outside this component so the
 * future campaign/meta layer can own economy without coupling it to environment art.
 */
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUHideoutProgressionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUHideoutProgressionComponent();

    UFUNCTION(BlueprintPure, Category="Hideout")
    int32 GetModuleLevel(ETUHideoutModuleType Type) const;

    UFUNCTION(BlueprintPure, Category="Hideout")
    bool IsModuleOperational(ETUHideoutModuleType Type) const { return GetModuleLevel(Type) > 0; }

    UFUNCTION(BlueprintCallable, Category="Hideout")
    bool SetModuleLevel(ETUHideoutModuleType Type, int32 NewLevel);

    UFUNCTION(BlueprintCallable, Category="Hideout")
    bool UpgradeModule(ETUHideoutModuleType Type);

    UFUNCTION(BlueprintPure, Category="Hideout")
    TArray<FTUHideoutModuleState> GetModules() const { return Modules; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hideout")
    TArray<FTUHideoutModuleState> Modules;

private:
    int32 FindModuleIndex(ETUHideoutModuleType Type) const;
};
