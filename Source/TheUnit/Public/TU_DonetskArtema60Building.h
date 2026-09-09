#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_DonetskArtema60Building.generated.h"

class UActorComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Reference-calibrated reconstruction of Artema Street 60, Donetsk.
 *
 * Documented facts are kept separate from working photo-match dimensions:
 * - original construction: 1928;
 * - original height: 3 storeys;
 * - current/postwar reconstruction: 4 storeys;
 * - semicircular street projection, tall stair-tower windows, white columns,
 *   decorative parapet and balcony balustrades are documented facade features.
 *
 * Width/depth/floor dimensions below are editable photo-match estimates until
 * reliable architectural drawings or survey measurements are available.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_DonetskArtema60Building : public AActor
{
    GENERATED_BODY()

public:
    ATU_DonetskArtema60Building();
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintPure, Category="Donetsk|Artema60|Reference")
    int32 GetDocumentedConstructionYear() const { return 1928; }

    UFUNCTION(BlueprintPure, Category="Donetsk|Artema60|Reference")
    int32 GetDocumentedOriginalStoreys() const { return 3; }

    UFUNCTION(BlueprintPure, Category="Donetsk|Artema60|Reference")
    int32 GetDocumentedCurrentStoreys() const { return 4; }

    UFUNCTION(BlueprintPure, Category="Donetsk|Artema60|Calibration")
    float GetEstimatedFrontageCm() const { return EstimatedFrontageCm; }

    UFUNCTION(BlueprintPure, Category="Donetsk|Artema60|Calibration")
    float GetEstimatedDepthCm() const { return EstimatedDepthCm; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60")
    TObjectPtr<USceneComponent> Root;

    /** Photo-match estimate from multi-angle public reference imagery. Not a survey dimension. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Calibration", meta=(ClampMin="3000.0", ClampMax="8000.0"))
    float EstimatedFrontageCm = 5200.0f;

    /** Photo-match estimate. Building footprint should be recalibrated if reliable plans become available. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Calibration", meta=(ClampMin="1000.0", ClampMax="4000.0"))
    float EstimatedDepthCm = 1900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Calibration", meta=(ClampMin="280.0", ClampMax="450.0"))
    float GroundFloorHeightCm = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Calibration", meta=(ClampMin="260.0", ClampMax="420.0"))
    float UpperFloorHeightCm = 340.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Calibration", meta=(ClampMin="250.0", ClampMax="800.0"))
    float RoundedProjectionRadiusCm = 430.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Calibration", meta=(ClampMin="300.0", ClampMax="1000.0"))
    float StairTowerWidthCm = 640.0f;

    /** Current reconstructed facade. Disable only when comparing against the 1930 historical reference. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Reference")
    bool bCurrentPostwarConfiguration = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Artema60|Debug")
    bool bGenerateWindowReferencePlates = true;

private:
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

    int32 GeneratedNameCounter = 0;

    void RebuildBuilding();
    void ClearGenerated();

    UStaticMeshComponent* AddBox(const FVector& Location, const FVector& Extents, const FString& BaseName,
        const FRotator& Rotation = FRotator::ZeroRotator);
    UStaticMeshComponent* AddCylinder(const FVector& Location, float RadiusCm, float HeightCm, const FString& BaseName);

    void BuildMainMass();
    void BuildStreetFacade();
    void BuildRoundedProjection();
    void BuildStairTower();
    void BuildPostwarDetails();
};