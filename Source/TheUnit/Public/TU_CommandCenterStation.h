#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_CommandCenterStation.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ATU_ArmedOperatorCharacter;
class UTUMissionPackageData;

UENUM(BlueprintType)
enum class ETUCommandCenterStationType : uint8
{
    Armory UMETA(DisplayName="Armory"),
    WeaponBench UMETA(DisplayName="Weapon Customization Bench"),
    Cage UMETA(DisplayName="Cage / Equipment"),
    UniformBench UMETA(DisplayName="Uniform / Gear Bench"),
    TestRange UMETA(DisplayName="Test Fire Range"),
    Briefing UMETA(DisplayName="Briefing Room"),
    MissionLaunch UMETA(DisplayName="Mission Launch")
};

/** Physical interaction point used inside the pre-mission Special Operations Command Center. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_CommandCenterStation : public AActor
{
    GENERATED_BODY()

public:
    ATU_CommandCenterStation();

    UFUNCTION(BlueprintCallable, Category="Command Center")
    virtual bool UseStation(ATU_ArmedOperatorCharacter* Operator);

    UFUNCTION(BlueprintCallable, Category="Command Center")
    void ConfigureStation(ETUCommandCenterStationType NewType, const FText& NewLabel, FName NewMissionId = NAME_None);

    UFUNCTION(BlueprintCallable, Category="Command Center|Mission")
    void SetMissionPackage(UTUMissionPackageData* InMissionPackage) { MissionPackage = InMissionPackage; }

    UFUNCTION(BlueprintPure, Category="Command Center")
    ETUCommandCenterStationType GetStationType() const { return StationType; }

    UFUNCTION(BlueprintPure, Category="Command Center")
    FText GetStationLabel() const { return StationLabel; }

    UFUNCTION(BlueprintPure, Category="Command Center|Mission")
    UTUMissionPackageData* GetMissionPackage() const { return MissionPackage; }

    UFUNCTION(BlueprintPure, Category="Command Center")
    bool IsOperatorInRange(const APawn* Pawn) const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command Center")
    TObjectPtr<UStaticMeshComponent> StationMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command Center")
    TObjectPtr<UBoxComponent> InteractionVolume;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center")
    ETUCommandCenterStationType StationType = ETUCommandCenterStationType::Armory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center")
    FText StationLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Mission")
    FName MissionId = NAME_None;

    /** Optional full data-driven package. Falls back to MissionId/StationLabel when unset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Mission")
    TObjectPtr<UTUMissionPackageData> MissionPackage = nullptr;
};
