#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_DonetskDistrictGenerator.generated.h"

class UActorComponent;
class UChildActorComponent;
class UDecalComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UTextRenderComponent;

/**
 * Procedural civilian Donetsk reference district for early layout/art validation.
 *
 * Building forms are derived from historical/public civilian architecture references
 * (Artema Street, Soviet residential typologies and the 2012 railway-station complex),
 * but the street arrangement is an original gameplay composition rather than a
 * current 1:1 operational map of a live conflict area.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_DonetskDistrictGenerator : public AActor
{
    GENERATED_BODY()

public:
    ATU_DonetskDistrictGenerator();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Independently calculated on each peer; collision-only and independent of render art. */
    UFUNCTION(BlueprintPure) FString GetGeneratedGeometrySignature() const;
    UFUNCTION(BlueprintPure) int32 GetGeneratedCollisionComponentCount() const;
    /** Render-only production art. These components must never own gameplay collision. */
    UFUNCTION(BlueprintPure) int32 GetProductionVisualComponentCount() const;
    /** Visible cube-based placeholders, excluding intentional road/ground surface carriers. */
    UFUNCTION(BlueprintPure) int32 GetVisiblePrimitiveFallbackCount() const;
    /** Deterministic render signature from production asset path plus relative transform. */
    UFUNCTION(BlueprintPure) FString GetGeneratedVisualSignature() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Donetsk")
    TObjectPtr<USceneComponent> Root;

    /** Dedicated reusable Artema 60 reconstruction; this replaces the old one-off blockout mass. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Donetsk|Reference Anchors")
    TObjectPtr<UChildActorComponent> Artema60Anchor;

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Donetsk|Layout", meta=(ClampMin="12000.0"))
    float DistrictWidthCm = 52000.0f;

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Donetsk|Layout", meta=(ClampMin="12000.0"))
    float DistrictLengthCm = 62000.0f;

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Donetsk|Layout")
    bool bGenerateReferenceLabels = true;

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Donetsk|Layout")
    bool bGenerateTransitFurniture = true;

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Donetsk|Layout")
    bool bGenerateUrbanVegetation = true;

    /** Fictional mission damage, intentionally separate from clean reference architecture. */
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Donetsk|Layout")
    bool bGenerateMissionDamage = true;

private:
    bool bInitialLayoutReceived = false;
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Khrush16Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Khrush14Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Khrush12Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Brezhnev14Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Brezhnev10Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Stalinka12Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> Stalinka10Mesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> StreetTreeAMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> StreetTreeBMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> BusShelterMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> SedanMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> RubblePileMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> CivicCoreMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> CivicWingMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> RailStationMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> IndustrialEdgeMesh;

    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> AsphaltMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PavingMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> GrassMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> SoilMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> RustMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ConcreteMaterial;

    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> AsphaltCrackDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PatchedAsphaltDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> TireWearDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> WaterStainDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> CurbGrimeDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> CrackedPlasterDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> RainStreakDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> RustDripsDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> UtilityMarkingDecalMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> FadedSignageDecalMaterial;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

    int32 GeneratedNameCounter = 0;

    void RebuildDistrict();
    void ClearGenerated();

    UStaticMeshComponent* AddBox(const FVector& Location, const FVector& Extents, const FString& BaseName,
        const FRotator& Rotation = FRotator::ZeroRotator);
    UStaticMeshComponent* AddHiddenBox(const FVector& Location, const FVector& Extents, const FString& BaseName,
        const FRotator& Rotation = FRotator::ZeroRotator);
    UStaticMeshComponent* AddProductionVisual(UStaticMesh* Asset, const FVector& Location,
        const FString& BaseName, const FRotator& Rotation = FRotator::ZeroRotator,
        const FVector& Scale = FVector::OneVector);
    UDecalComponent* AddHeroDecal(UMaterialInterface* Material, const FVector& Location,
        const FVector& DecalSize, const FString& BaseName, const FRotator& Rotation);
    void AddLabel(const FString& Text, const FVector& Location, const FRotator& Rotation = FRotator(0.0f, 90.0f, 0.0f));

    void BuildRoadNetwork();
    void BuildCentralSquareReference();
    void BuildKhrushchyovkaCourtyard();
    void BuildBrezhnevkaBlocks();
    void BuildStalinistStreetWall();
    void BuildRailStationReference();
    void BuildIndustrialEdge();
    void BuildStreetFurniture();
    void BuildUrbanVegetation();
    void BuildMissionDamageLayer();
    void BuildHeroSliceDressing();

    void BuildSimpleFacadeBlock(const FVector& Origin, int32 Floors, int32 Bays, float BayWidthCm,
        float DepthCm, float FloorHeightCm, const FString& Prefix, bool bBalconies, bool bRaisedGroundFloor);
};
