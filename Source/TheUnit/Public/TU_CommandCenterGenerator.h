#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_CommandCenterStation.h"
#include "TU_CommandCenterGenerator.generated.h"

class UActorComponent;
class UArrowComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTUMissionPackageData;

/**
 * Editor-visible graybox for The Unit's walkable pre-mission headquarters.
 *
 * Visual language is intentionally utilitarian/institutional: a secure corridor,
 * fluorescent/drop-ceiling feel, separate Armory, Cage, Briefing and Test Range
 * spaces, and physical interaction stations instead of an abstract front-end menu.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_CommandCenterGenerator : public AActor
{
    GENERATED_BODY()

public:
    ATU_CommandCenterGenerator();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Command Center|Mission")
    void SetDefaultMissionPackage(UTUMissionPackageData* InMissionPackage) { DefaultMissionPackage = InMissionPackage; }

    UFUNCTION(BlueprintPure, Category="Command Center|Mission")
    UTUMissionPackageData* GetDefaultMissionPackage() const { return DefaultMissionPackage; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command Center")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Dimensions", meta=(ClampMin="1200.0"))
    float HubWidth = 5200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Dimensions", meta=(ClampMin="1800.0"))
    float HubLength = 7600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Dimensions", meta=(ClampMin="260.0"))
    float CeilingHeight = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Dimensions", meta=(ClampMin="250.0"))
    float CorridorWidth = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Debug")
    bool bGenerateLabels = true;

    /** Spawn functional interaction actors at the authored station markers during BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Stations")
    bool bSpawnRuntimeStations = true;

    /** Full mission package injected into generated Briefing and Mission Launch stations. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Command Center|Mission")
    TObjectPtr<UTUMissionPackageData> DefaultMissionPackage = nullptr;

private:
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

    UPROPERTY(Transient)
    TArray<TObjectPtr<ATU_CommandCenterStation>> RuntimeStations;

    struct FStationSpawn
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
        ETUCommandCenterStationType Type = ETUCommandCenterStationType::Armory;
        FString Label;
        FName MissionId = NAME_None;
    };

    TArray<FStationSpawn> StationSpawns;

    void BuildHub();
    void ClearGeneratedComponents();
    void ClearRuntimeStations();

    UStaticMeshComponent* AddCube(const FVector& Location, const FVector& Extents, const FName& Name, const FRotator& Rotation = FRotator::ZeroRotator);
    UArrowComponent* AddMarker(const FVector& Location, const FRotator& Rotation, const FName& Name);
    void AddLabel(const FString& Text, const FVector& Location, const FRotator& Rotation = FRotator(0.0f, 90.0f, 0.0f));
    void AddStationMarker(ETUCommandCenterStationType Type, const FString& Label, const FVector& Location, const FRotator& Rotation, FName MissionId = NAME_None);

    void BuildSecureCorridor();
    void BuildArmory();
    void BuildCage();
    void BuildBriefingRoom();
    void BuildTestRange();
    void BuildOperationsDetails();
};
